#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "log.h"
#include "LSLogMemPool.h"
#include "LSLogTemplate.h"
#include "LSLogFile.h"

#define LSLOG_INC(i)			((i + 1) % LSLOG_MAX_LOG_NUM )

#define LSLOG_DEC(i, hdr)	\
	do {					\
		if (i == 0) { 		\
		}					\
		else {				\
							\
		}					\
	} while (0)

#define ITEM_OFF 				sizeof(struct LogFileHeader)
#define ITEM_SIZE 				sizeof(LogStorageItem)
#define ITEM_AT(mAddr, i)  	\
	i < 0 ? NULL : ((LogStorageItem *)((char *)(mAddr) + ITEM_OFF) + i)
#define ITEM_HEAD_PTR(mAddr)	ITEM_AT(mAddr, 0)
#define ITEM_TAIL_PTR(mAddr)	ITEM_AT(mAddr, LSLOG_MAX_LOG_NUM - 1)
#define ITEM_LAST_PTR(mAddr, hdr)	ITEM_AT(mAddr, hdr.currentIndex)

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
		isNewFile = true;
		fileHeader.loopCover = 0;
		fileHeader.currentIndex = -1;
	}
	else {
		isNewFile = false;
	}
	logFileFd = open(logPath, O_RDWR | O_CREAT, LSLOG_PERM);
	if (logFileFd < 0) {
		myLogErr("open %s for log failed", logPath);
		exit(-1);
	}

	logFileSize = getLogFileSize();
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
	LogFileHeader *header = (LogFileHeader *)mapAddr;
	if (isNewFile) {
		memcpy(header, &fileHeader, sizeof(LogFileHeader));
	}
	else {
		memcpy(&fileHeader, header, sizeof(LogFileHeader));
	}
	pthread_rwlock_init(&rwlock, NULL);
}

LSLogFile::~LSLogFile()
{
	unsigned fileSize; 

	fileSize = getLogFileSize();
	msync(mapAddr, fileSize, MS_ASYNC); 
	munmap(mapAddr, fileSize);
	close(logFileFd);

	pthread_rwlock_destroy(&rwlock);

	myLog("destruct LSLogTemplate");
}

unsigned LSLogFile::getLogFileSize()
{
	return ITEM_OFF + LSLOG_MAX_LOG_NUM * sizeof(LogStorageItem);
}

bool LSLogFile::save(LSLogInfo *logInfo)
{
	LogStorageItem newLogItem;

	// 转成模板
	logTpl->shrink(logInfo, &newLogItem);

	pthread_rwlock_wrlock(&rwlock);
	LogStorageItem *lastLogItem = ITEM_LAST_PTR(mapAddr, fileHeader);
	if (newLogItem.t < lastLogItem->t) {
		// TODO: 找到合适的插入位置
		int newIndex = searchBigerIndex(newLogItem.t);
		if (fileHeader.currentIndex < newIndex && fileHeader.loopCover)
			fileHeader.loopCover = false;
		fileHeader.currentIndex = newIndex;
		// 丢弃后面的index(更改header.currentIndex)
	}
	else {
		fileHeader.currentIndex = LSLOG_INC(fileHeader.currentIndex);
		lastLogItem = ITEM_AT(mapAddr, fileHeader.currentIndex);
		memcpy(lastLogItem, &newLogItem, sizeof(LogStorageItem));
		if (fileHeader.currentIndex == 0) {
			fileHeader.loopCover = 1;
		}
		memcpy(mapAddr, &fileHeader, sizeof(LogFileHeader));
	}
	pthread_rwlock_unlock(&rwlock);

	myLog("saved finish");
	return true;
}

int LSLogFile::query(time_t from, time_t to, int blockSize, int blockIndex, struct LSLogInfo *& logInfos)
{
	int i, offIndex, index, totalInfo;
	int beginIndex, endIndex;
	LogStorageItem *item;		
	LSLogInfo *logInfo, *prev;

	prev = NULL;
	pthread_rwlock_rdlock(&rwlock);
	beginIndex = searchBigerIndex(from);
	endIndex = searchSmallerIndex(to);

	if (endIndex >= beginIndex) {
		totalInfo = (endIndex - beginIndex) + 1;
	}
	else {
		totalInfo = (LSLOG_MAX_LOG_NUM - 1 - beginIndex) + 1 + endIndex + 1;
	}

	// 得到指定的记录
	offIndex = (blockIndex - 1) * blockSize;
	for (i=0; i<blockSize; i++) {
		index = (beginIndex + offIndex + i) % LSLOG_MAX_LOG_NUM; 
		if (endIndex >= beginIndex) {
			if (index > endIndex) break;
		}
		else {
			if (index < endIndex) break;
		}

		item = ITEM_AT(mapAddr, index);
		logInfo = memPool->malloc();	
		logTpl->expand(item, logInfo);
		if (prev == NULL) {
			prev = logInfo;
		}
		else {
			prev->next = logInfo;
		}
	}

	pthread_rwlock_unlock(&rwlock);
	return totalInfo;
}

int LSLogFile::searchBigerIndex(time_t key)
{
	return 0;	
}

int LSLogFile::searchSmallerIndex(time_t key)
{
	return 0;
}

