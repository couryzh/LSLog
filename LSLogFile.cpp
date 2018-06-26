#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "log.h"
#include "LSLogMemPool.h"
#include "LSLogTemplate.h"
#include "LSLogFile.h"

#define ITEM_OFF 				sizeof(struct LogFileHeader)

#define ITEM_SIZE 				sizeof(LogStorageItem)
#define ITEM_AT(mAddr, i)  		(LogStorageItem *)((char *)(mAddr) + ITEM_OFF) + (i)
#define ITEM_HEAD_PTR(mAddr)	ITEM_AT(mAddr, 0)
#define ITEM_TAIL_PTR(mAddr)	ITEM_AT(mAddr, LSLOG_MAX_LOG_NUM - 1)

#define LSLOG_PERM 		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH

LSLogFile::LSLogFile(unsigned type, LSLogMemPool *pool, LSLogTemplate *tpl)
	: logTpl(tpl), memPool(pool)
{
	unsigned logFileSize;
	const char *fileName;

	switch (type) {
		case OPE_LOG:
			fileName = LSLOG_OPE_FILE;
			break;
		case CFG_LOG:
			fileName = LSLOG_CFG_FILE;
			break;
		case RUN_LOG:
			fileName = LSLOG_RUN_FILE;
			break;
		case ABNOR_LOG:
			fileName = LSLOG_ABNOR_FILE;
			break;
		default:
			fileName = (char *)"ls.log";
			break;
	}
	
	if (snprintf(logPath, LSLOG_MAX_PATH_LEN, "%s%s", LSLOG_WORK_PATH, fileName) >= LSLOG_MAX_PATH_LEN) {
		myLog("too long path: %s%s",  LSLOG_WORK_PATH, fileName); 
		exit(-1);
	}
	if (access(logPath, F_OK) < 0) {
		isCreateFile = true;
		fileHeader.overed = 0;
		fileHeader.currentIndex = 0;
	}
	else {
		isCreateFile = false;
	}
	logFileFd = open(logPath, O_RDWR | O_CREAT, LSLOG_PERM);
	if (logFileFd < 0) {
		myLogErr("open %s for log failed", logPath);
		exit(-1);
	}

	logFileSize = sizeof(LogFileHeader) + LSLOG_MAX_LOG_NUM * sizeof(LogStorageItem);
	if (ftruncate(logFileFd, logFileSize) < 0) {
		close(logFileFd);
		myLog("expand logfile size to %u\n", logFileSize);
		exit(-1);
	}

	mapAddr = mmap(0, logFileSize, PROT_READ|PROT_WRITE, MAP_SHARED, logFileFd, 0); 
	if (mapAddr == MAP_FAILED) {
		close(logFileFd);
		myLogErr("mmap logfile");
		exit(-1);
	}

	// 更新本地头, 保证本地的文件头副本与文件中的头部一致
	if (isCreateFile) {
		memcpy(mapAddr, &fileHeader, sizeof(LogFileHeader));
	}
	else {
		memcpy(&fileHeader, mapAddr, sizeof(LogFileHeader));
	}

	char tplPath[LSLOG_MAX_PATH_LEN];
	if (snprintf(tplPath, LSLOG_MAX_PATH_LEN, "%s%s", LSLOG_WORK_PATH, LSLOG_CFG_TEMPLATE) >= LSLOG_MAX_PATH_LEN) {
		myLog("too long path: %s%s",  LSLOG_WORK_PATH, LSLOG_CFG_TEMPLATE); 
		exit(-1);
	}
}

LSLogFile::~LSLogFile()
{
	unsigned  logFileSize; 

	logFileSize = sizeof(LogFileHeader) + LSLOG_MAX_LOG_NUM * sizeof(LogStorageItem);
	msync(mapAddr, logFileSize, MS_ASYNC); 
	munmap(mapAddr, logFileSize);

	myLog("destruct LSLogTemplate");
}

bool LSLogFile::save(LSLogInfo *logInfo)
{
	LogStorageItem logStorageItem, *storageAddr;

	// 转成模板
	logTpl->shrink(logInfo, &logStorageItem);

	// 更新本地头, 保证本地的文件头副本与文件中的头部一致
	fileHeader.currentIndex = (fileHeader.currentIndex + 1) % LSLOG_MAX_LOG_NUM;
	if (fileHeader.currentIndex == 0) fileHeader.overed = 1;
	storageAddr  = ITEM_AT(mapAddr, fileHeader.currentIndex);

	// 写入记录, 更新文件头
	memcpy(storageAddr, &logStorageItem, sizeof(LogStorageItem));
	LogFileHeader *header = (LogFileHeader *)mapAddr;
	memcpy(header, &fileHeader, sizeof(LogFileHeader));

	myLog("saved finish");
	return true;
}

int LSLogFile::query(time_t from, time_t to, int blockSize, int blockIndex, struct LSLogInfo *& logInfos)
{
	int i, index, totalInfo;
	LogStorageItem *item, *begin, *end;		

	begin = searchIndex(from);
	end = searchIndex(to);

	if (end >= begin)
		totalInfo = (end - begin) / ITEM_SIZE + 1;
	else 
		totalInfo = (ITEM_TAIL_PTR(mapAddr) - begin) / ITEM_SIZE + 1 +
			(end - ITEM_HEAD_PTR(mapAddr)) / ITEM_SIZE + 1;

	// 得到指定的记录
	index = (blockIndex - 1) * blockSize;
	for (i=0; i<blockSize; i++) {
		item = ITEM_AT(mapAddr, index);
		if (item > end) break;

		// fill  into logInfos	
		index = (index+1) % LSLOG_MAX_LOG_NUM;
	}

	return totalInfo;
}

LogStorageItem * LSLogFile::searchIndex(time_t key)
{
	return NULL;	
}



