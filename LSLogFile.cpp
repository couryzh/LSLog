#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "LSLogFile.h"
#include "LSLogTemplate.h"

#define ITEM_OFF 				sizeof(struct LogFileHeader)

#define ITEM_SIZE 				sizeof(LogStorageItem)
#define ITEM_AT(mAddr, i)  		(LogStorageItem *)((char *)(mAddr) + ITEM_OFF) + (i)
#define ITEM_HEAD_PTR(mAddr)	ITEM_AT(mAddr, 0)
#define ITEM_TAIL_PTR(mAddr)	ITEM_AT(mAddr, LSLOG_MAX_LOG_NUM - 1)

LSLogFile::LSLogFile()
{
	unsigned logFileSize;

	snprintf(logPath, 128, "%s%s", LSLOG_WORK_PATH, LSLOG_FILE);	   
	logFile = open(logPath, O_RDWR | O_CREAT);
	if (logFile < 0) {
		fprintf(stderr, "open %s for log failed: %s\n", logPath, strerror(errno));
		exit(-1);
	}

	logFileSize = sizeof(LogFileHeader) + LSLOG_MAX_LOG_NUM * sizeof(LogStorageItem);
	if (ftruncate(logFile, logFileSize) < 0) {
		close(logFile);
		fprintf(stderr, "expand logfile size to %u\n", logFileSize);
		exit(-1);
	}

	mapAddr = mmap(0, logFileSize, PROT_READ|PROT_WRITE, MAP_SHARED, logFile, 0); 
	if (mapAddr == MAP_FAILED) {
		close(logFile);
		fprintf(stderr, "expand logfile size to %u\n", logFileSize);
		exit(-1);
	}

	logtpl = new LSLogTemplate(LSLOG_CFG_TEMPLATE);
}

bool LSLogFile::save(LSLogInfo *logInfo)
{
	LogStorageItem logStorageItem, *storageAddr;

	logtpl->shrink(logInfo, &logStorageItem);

	LogFileHeader *header = (LogFileHeader *)mapAddr;
	header->currentIndex = (header->currentIndex + 1) % LSLOG_MAX_LOG_NUM;
	storageAddr  = ITEM_AT(mapAddr, header->currentIndex);
	memcpy(storageAddr, &logStorageItem, sizeof(LogStorageItem));
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

LSLogFile::~LSLogFile()
{
	unsigned  logFileSize; 

	logFileSize = sizeof(LogFileHeader) + LSLOG_MAX_LOG_NUM * sizeof(LogStorageItem);
	msync(mapAddr, logFileSize, MS_ASYNC); 
	munmap(mapAddr, logFileSize);

	delete logtpl;
}

