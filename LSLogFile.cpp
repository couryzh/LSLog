#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "log.h"
#include "LSLogMemPool.h"
#include "LSLogTemplate.h"
#include "LSLogFile.h"


#define LSLOG_MAX_LOG_NUM_ 		(LSLOG_MAX_LOG_NUM + 1)

#define LSLOG_INC(i)			((i + 1) % LSLOG_MAX_LOG_NUM_)
#define LSLOG_DEC(i)			((i + LSLOG_MAX_LOG_NUM_ - 1) % LSLOG_MAX_LOG_NUM_)

#define ITEM_OFF 					sizeof(struct LogFileHeader)
#define ITEM_SIZE 					sizeof(LogStorageItem)
#define LAST_ITEM(hdr) 				((hdr.tailIndex + LSLOG_MAX_LOG_NUM_) - 1) % LSLOG_MAX_LOG_NUM_ 
#define ITEM_AT(mAddr, i)  	\
	(i) < 0 ? NULL : ((LogStorageItem *)((char *)(mAddr) + ITEM_OFF) + (i))
#define ITEM_HEAD_PTR(mAddr)		ITEM_AT(mAddr, 0)
#define ITEM_TAIL_PTR(mAddr)		ITEM_AT(mAddr, LSLOG_MAX_LOG_NUM_ - 1)

#define LSLOG_PERM 					S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH


LSLogFile::LSLogFile(unsigned type, LSLogMemPool *pool, LSLogTemplate *tpl)
	: logType(type), logTpl(tpl), memPool(pool)
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
		fileHeader.headIndex= fileHeader.tailIndex = -1;
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
		myLog("expand logfile size to %u", logFileSize);
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

	//myLog("destruct LSLogTemplate");
}

unsigned LSLogFile::getLogFileSize()
{
	return ITEM_OFF + LSLOG_MAX_LOG_NUM_ * sizeof(LogStorageItem);
}

void LSLogFile::unset(int fromIndex, int toIndex)
{
	int i;
	LogStorageItem  *item;

	myLog("unset from %d to %d", fromIndex, toIndex);
	if (fromIndex <= toIndex) {
		for (i=fromIndex; i<=toIndex; i++) {
			item = ITEM_AT(mapAddr, i);
			item->t = 0;
		}
	}
	else {
		for (i=fromIndex; i<LSLOG_MAX_LOG_NUM_; i++) {
			item = ITEM_AT(mapAddr, i);
			item->t = 0;
		}

		for (i=0; i<=toIndex; i++) {
			item = ITEM_AT(mapAddr, i);
			item->t = 0;
		}
	}
}

void LSLogFile::dumpStorageItem(LogStorageItem *item)
{
	printf("t: %d user: %d tpl: %s\n", (int)item->t, item->user, item->eventTpl);
}

bool LSLogFile::save(LSLogInfo *logInfo)
{
	int lastIndex;
	LogStorageItem newLogItem, *lastLogItem ;

	//myLog("save begin");
	// 转成模板
	if (!logTpl->shrink(logInfo, &newLogItem)) {
		myLog("faile to convert template");
		return false;
	}
	//dumpStorageItem(&newLogItem);

	pthread_rwlock_wrlock(&rwlock);
	//memcpy(&fileHeader, (LogFileHeader*)mapAddr, sizeof(LogFileHeader));

	if (fileHeader.tailIndex == -1) {
		fileHeader.headIndex = 0;
		fileHeader.tailIndex = 0;
		lastLogItem = ITEM_AT(mapAddr, fileHeader.tailIndex);
		memcpy(lastLogItem, &newLogItem, sizeof(LogStorageItem));
		fileHeader.tailIndex = LSLOG_INC(fileHeader.tailIndex);
		//myLog(">>> headIndex: %d tailIndex: %d", fileHeader.headIndex, fileHeader.tailIndex);
		memcpy(mapAddr, &fileHeader, sizeof(LogFileHeader));
		pthread_rwlock_unlock(&rwlock);
		return true;
	}

	lastIndex = LAST_ITEM(fileHeader);
	lastLogItem = ITEM_AT(mapAddr, lastIndex);
	if (newLogItem.t < lastLogItem->t) {
		//myLog("222222222");
		// 找到合适的插入位置
		int newIndex = searchLeft(newLogItem.t);
		if (newIndex == fileHeader.headIndex) {
			fileHeader.headIndex = 0;
			fileHeader.tailIndex = 0;
		}

		// 丢弃后面的index
		unset(newIndex, lastIndex);	

		lastLogItem = ITEM_AT(mapAddr, newIndex);	
		memcpy(lastLogItem, &newLogItem, sizeof(LogStorageItem));
		fileHeader.tailIndex = LSLOG_INC(newIndex);
		memcpy(mapAddr, &fileHeader, sizeof(LogFileHeader));
	}
	else {
		//myLog("11111111111");
		lastLogItem = ITEM_AT(mapAddr, fileHeader.tailIndex);
		memcpy(lastLogItem, &newLogItem, sizeof(LogStorageItem));
		fileHeader.tailIndex = LSLOG_INC(fileHeader.tailIndex);
		if (fileHeader.tailIndex == fileHeader.headIndex)
			fileHeader.headIndex = LSLOG_INC(fileHeader.headIndex);
		memcpy(mapAddr, &fileHeader, sizeof(LogFileHeader));
	}
	pthread_rwlock_unlock(&rwlock);

	//myLog("saved finish");
	myLog("=== save (t=%d), headIndex: %d tailIndex: %d", (int)logInfo->t, fileHeader.headIndex, fileHeader.tailIndex);
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
	logInfos = NULL;
	//memcpy(&fileHeader, (LogFileHeader*)mapAddr, sizeof(LogFileHeader));

	//myLog("from %d to %d", (int)from, (int)to);
	beginIndex = searchLeft(from);
	endIndex = searchRight(to);
	myLog("beginIndex=%d, endIndex=%d", beginIndex, endIndex);

	if (endIndex >= beginIndex) {
		totalInfo = (endIndex - beginIndex) + 1;
	}
	else {
		totalInfo = endIndex + LSLOG_MAX_LOG_NUM - beginIndex + 1;
	}

	// 得到指定的记录
	offIndex = (blockIndex - 1) * blockSize;
	for (i=0; i<blockSize; i++) {
		index = (beginIndex + offIndex + i) % LSLOG_MAX_LOG_NUM_; 
		if (endIndex >= beginIndex) {
			if (index > endIndex) break;
		}
		else {
			if (index < endIndex) break;
		}

		item = ITEM_AT(mapAddr, index);
		logInfo = memPool->malloc();	
		if (!logTpl->expand(item, logInfo)) {
			myLog("failed to expand %s", item->eventTpl);
			memPool->free(logInfo);
			continue;
		}
		if (prev == NULL) {
			logInfos = prev = logInfo;
			continue;
		}
		prev->next = logInfo;
		prev = logInfo;
	}

	pthread_rwlock_unlock(&rwlock);
	return totalInfo;
}

int LSLogFile::searchLeft(time_t key)
{
	int i, last;
	LogStorageItem  *item;

	if (fileHeader.headIndex < 0 || fileHeader.tailIndex < 0) 
		return 0;

	last = LAST_ITEM(fileHeader);
	item = ITEM_AT(mapAddr, last);
	if (item->t <= key)
		return last;

	for (i=fileHeader.headIndex; i != fileHeader.tailIndex; i=LSLOG_INC(i)) {
		item = ITEM_AT(mapAddr, i);
		if (item->t > key)
			break;
	}
	return i;
}

int LSLogFile::searchRight(time_t key)
{
	int i;
	LogStorageItem  *item;

	if (fileHeader.headIndex < 0 || fileHeader.tailIndex < 0) 
		return 0;

	item = ITEM_AT(mapAddr, fileHeader.headIndex);
	if (item->t > key)
		return fileHeader.headIndex;

	for (i=LAST_ITEM(fileHeader); i!= fileHeader.headIndex; i=LSLOG_DEC(i)) {
		item = ITEM_AT(mapAddr, i);
		if (item->t <= key) {
			break;
		}
	}

	return i;
}

#if 1
void LSLogFile::dump()
{
	int i;
	LogStorageItem  *item;
	LogFileHeader *header;
	
	header = (LogFileHeader*) mapAddr;
	printf("----------------------------\n"
		   "headIndex: %d\n"
		   "tailIndex: %d\n"
		   "----------------------------\n",
			header->headIndex,
			header->tailIndex);
	for (i=0; i<LSLOG_MAX_LOG_NUM_; i++) {
		item = ITEM_AT(mapAddr, i); 
		printf("0x%p: t: %d user: %d eventtpl: %s\n", 
				item, (int)item->t, item->user, item->eventTpl);
	}
	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n");
}
#endif
