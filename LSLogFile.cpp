#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include "log.h"
#include "LSLogMemPool.h"
#include "LSLogTemplate.h"
#include "LSLogFile.h"

#define LSLOG_INC(i)		((i + 1) % LSLOG_MAX_LOG_NUM)
#define LSLOG_DEC(i)		((i + LSLOG_MAX_LOG_NUM - 1) % LSLOG_MAX_LOG_NUM)

#define HDR_SIZE			sizeof(struct LogFileHeader)
#define ITEM_SIZE 			sizeof(LogStorageItem)
#define HDR_MAP_OFF(index)  sizeof(short) + sizeof(LogItemMap) * index
#define ITEM_OFF(index)		HDR_SIZE + sizeof(LogStorageItem) * index
#define NEXT_INDEX(hdr)		hdr.logItemMap[hdr.nextMapPos].index
#define LAST_OFF(hdr)		LSLOG_DEC(hdr.nextMapPos)
#define LAST_INDEX(hdr)     hdr.logItemMap[LAST_OFF(hdr)].index

#define INVALID_T			0 //0x7fffffff

#define LAST_ITEM(hdr)			0

#if 0
#define ITEM_AT(mAddr, i)  	\
	(i) < 0 ? NULL : ((LogStorageItem *)((char *)(mAddr) + ITEM_OFF) + (i))
#define ITEM_HEAD_PTR(mAddr)		ITEM_AT(mAddr, 0)
#define ITEM_TAIL_PTR(mAddr)		ITEM_AT(mAddr, LSLOG_MAX_LOG_NUM_ - 1)
#endif

#define LSLOG_PERM 					S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH


LSLogFile::LSLogFile(unsigned type, LSLogMemPool *pool, LSLogTemplate *tpl)
	: logType(type), logTpl(tpl), memPool(pool)
{
	unsigned logFileSize;
	const char *fileName;
	int logFileFd;
	bool isNewFile;

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
	
	if (snprintf(logPath, LSLOG_MAX_PATH_LEN+1, "%s%s", LSLOG_WORK_PATH, fileName) >= LSLOG_MAX_PATH_LEN+1) {
		myLog("too long path: %s%s",  LSLOG_WORK_PATH, fileName); 
		exit(-1);
	}

	if (access(logPath, F_OK) < 0) {
		isNewFile = true;
	}
	else {
		isNewFile = false;
	}
	logFile = fopen(logPath, "wb+");
	if (!logFile) {
		myLogErr("open %s for log failed", logPath);
		exit(-1);
	}

	logFileSize = getLogFileSize();
	logFileFd = fileno(logFile);
	if (ftruncate(logFileFd, logFileSize) < 0) {
		fclose(logFile);
		myLog("expand logfile size to %u", logFileSize);
		exit(-1);
	}

	pthread_rwlock_init(&rwlock, NULL);

	if (isNewFile) {
		int i;
		fileHeader.nextMapPos= 0;
		for (i=0; i<LSLOG_MAX_LOG_NUM; i++) {
			fileHeader.logItemMap[i].t = INVALID_T;
			fileHeader.logItemMap[i].index = i;
		}
		dumpHeader();
	}
	else {
		loadHeader();
	}
	//myLog("initinitinit");
	//print();

	auxItemMap = new LogItemMap[LSLOG_MAX_LOG_NUM/2 + 1];
	if (auxItemMap == NULL) {
		myLog("alloc auxItemMap failed");
		exit(-1);
	}
}

LSLogFile::~LSLogFile()
{
	fclose(logFile);

	pthread_rwlock_destroy(&rwlock);

	//myLog("destruct LSLogTemplate");
}

bool LSLogFile::loadHeader()
{
	fseek(logFile, 0, SEEK_SET);
	return fread(&fileHeader, sizeof(LogFileHeader), 1, logFile) == 1;
}

bool LSLogFile::dumpHeader()
{
	fseek(logFile, 0, SEEK_SET);
	return fwrite(&fileHeader, sizeof(LogFileHeader), 1,logFile)== 1;
}

unsigned LSLogFile::getLogFileSize()
{
	return HDR_SIZE + LSLOG_MAX_LOG_NUM * ITEM_SIZE;
}

void LSLogFile::unset(int fromIndex, int toIndex)
{
#if 0
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
#endif
}

void LSLogFile::printStorageItem(LogStorageItem *item)
{
	printf("t: %d user: %s tpl: %s\n", (int)item->t, item->user, item->eventTpl);
}

bool LSLogFile::save(LSLogInfo *logInfo)
{
	int lastOffset;
	short nextMapIndex, newIndex, lastIndex;
	long itemOffset;
	LogStorageItem newLogItem;

	//myLog("save begin");

	// 转成模板
	if (!logTpl->shrink(logInfo, &newLogItem)) {
		myLog("faile to convert template");
		return false;
	}
	//printStorageItem(&newLogItem);

	pthread_rwlock_wrlock(&rwlock);
	//memcpy(&fileHeader, (LogFileHeader*)mapAddr, sizeof(LogFileHeader));

	nextMapIndex = NEXT_INDEX(fileHeader);
	lastIndex = LAST_INDEX(fileHeader);
	myLog("nextMapIndex: %hd,  lastIndex: %hd", nextMapIndex, lastIndex);
	if (newLogItem.t < fileHeader.logItemMap[lastIndex].t) {
		if (fileHeader.logItemMap[nextMapIndex].t != INVALID_T) {
			lastOffset = LAST_OFF(fileHeader);
			if (lastOffset <= LSLOG_MAX_LOG_NUM / 2) {
				memcpy(auxItemMap, &fileHeader.logItemMap[0], sizeof(LogItemMap) * (lastOffset + 1)); 	
				memcpy(&fileHeader.logItemMap[0], &fileHeader.logItemMap[nextMapIndex], sizeof(LogItemMap) * (LSLOG_MAX_LOG_NUM - nextMapIndex));
				memcpy(&fileHeader.logItemMap[LSLOG_MAX_LOG_NUM - nextMapIndex], auxItemMap, sizeof(LogItemMap) * (lastOffset + 1));
				fileHeader.nextMapPos = 0;
			}
			else{
				memcpy(auxItemMap, &fileHeader.logItemMap[nextMapIndex], sizeof(LogItemMap) * (LSLOG_MAX_LOG_NUM - nextMapIndex));
				memcpy(&fileHeader.logItemMap[nextMapIndex], &fileHeader.logItemMap[0], sizeof(LogItemMap) * (lastOffset + 1));
				memcpy(&fileHeader.logItemMap[0], auxItemMap, sizeof(LogItemMap) * (LSLOG_MAX_LOG_NUM - nextMapIndex));
				fileHeader.nextMapPos = 0;
			}
		}

		newIndex = searchLeft(newLogItem.t);
		assert(newIndex >= 0);
		for (short i=newIndex; i<LSLOG_MAX_LOG_NUM; i++) {
			if (fileHeader.logItemMap[i].t == INVALID_T)
				break;
			fileHeader.logItemMap[i].t = INVALID_T;
		}
		fileHeader.logItemMap[newIndex].t = (int)newLogItem.t;
		fileHeader.nextMapPos = LSLOG_INC(newIndex);

		itemOffset = ITEM_OFF(newIndex);
		fseek(logFile, itemOffset, SEEK_SET);
		fwrite(&newLogItem, sizeof(LogStorageItem), 1, logFile);
		dumpHeader();
	}
	else {
		itemOffset = ITEM_OFF(nextMapIndex);
		fseek(logFile, itemOffset, SEEK_SET);
		fwrite(&newLogItem, sizeof(LogStorageItem), 1, logFile);

		fileHeader.logItemMap[nextMapIndex].t = newLogItem.t;
		fileHeader.nextMapPos = LSLOG_INC(fileHeader.nextMapPos); 
		fseek(logFile, 0, SEEK_SET);
		fwrite(&fileHeader.nextMapPos, sizeof(short), 1, logFile);
		fseek(logFile, HDR_MAP_OFF(nextMapIndex), SEEK_SET);
		fwrite(&fileHeader.logItemMap[nextMapIndex], sizeof(LogItemMap), 1, logFile);
	}
	fflush(logFile);	
	pthread_rwlock_unlock(&rwlock);

	//myLog("saved finish");
	return true;
}

int LSLogFile::query(time_t from, time_t to, int blockSize, int blockIndex, struct LSLogInfo *& logInfos)
{
	int i, offIndex, index, totalInfo;
	int beginIndex, endIndex;
	long offset;
	LogStorageItem item;
	LSLogInfo *logInfo, *prev;

	prev = NULL;
	pthread_rwlock_rdlock(&rwlock);
	logInfos = NULL;
	//memcpy(&fileHeader, (LogFileHeader*)mapAddr, sizeof(LogFileHeader));

	//myLog("from %d to %d", (int)from, (int)to);
	beginIndex = searchLeft(from);
	endIndex = searchRight(to);
	myLog("beginIndex=%d, endIndex=%d", beginIndex, endIndex);
	// 无对应时间段数据
	if (beginIndex < 0 || endIndex < 0) return 0;

	if (endIndex >= beginIndex) {
		totalInfo = (endIndex - beginIndex) + 1;
	}
	else {
		totalInfo = endIndex + LSLOG_MAX_LOG_NUM - beginIndex + 1;
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
		offset = ITEM_OFF(index);
		fseek(logFile, offset, SEEK_SET);
		if (fread(&item, sizeof(LogStorageItem), 1, logFile) < 0) {
			myLog("failed to get LogStorageItem");
			continue;
		}
		logInfo = memPool->malloc();	
		if (!logTpl->expand(&item, logInfo)) {
			myLog("failed to expand %s", item.eventTpl);
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

short LSLogFile::searchLeft(time_t key)
{
	short i, headIndex, tailIndex;

	if (fileHeader.logItemMap[NEXT_INDEX(fileHeader)].t == INVALID_T) {
		headIndex = 0;
		tailIndex = LAST_INDEX(fileHeader);
	}
	else {
		headIndex = NEXT_INDEX(fileHeader);
		tailIndex = LAST_INDEX(fileHeader);
	}
	myLog("headIndex: %hd tailIndex: %hd", headIndex, tailIndex);
	if (fileHeader.logItemMap[tailIndex].t < (int)key) {
		return -1;
	}

	for (i=headIndex; i!=tailIndex; i=LSLOG_INC(i)) {
		if (fileHeader.logItemMap[i].t > (int)key)
			break;
	}
	return i;
}

short LSLogFile::searchRight(time_t key)
{
	short i, headIndex, tailIndex;

	if (fileHeader.logItemMap[NEXT_INDEX(fileHeader)].t == INVALID_T) {
		headIndex = 0;
		tailIndex = NEXT_INDEX(fileHeader);
	}
	else {
		headIndex = NEXT_INDEX(fileHeader);
		tailIndex = LAST_INDEX(fileHeader);
	}

	if (fileHeader.logItemMap[headIndex].t > (int)key) {
		return -1;
	}
	for (i=tailIndex; i!=headIndex; i=LSLOG_DEC(i)) { 
		if (fileHeader.logItemMap[i].t <= (int)key)
			break;
	}

	return i;
}

#if 1
void LSLogFile::print()
{
	int i;
	LogItemMap *itemMap;
	LogStorageItem  item;
	LogFileHeader header;
	
	fseek(logFile, 0, SEEK_SET);
	fread(&header, sizeof(LogFileHeader), 1, logFile);

	printf("----------------------------\n"
		   "nextMapPos: %d\n", header.nextMapPos);
	for (i=0; i<LSLOG_MAX_LOG_NUM; i++) {
		itemMap = &fileHeader.logItemMap[i];
		printf("t: %d index: %hd\n", 
				itemMap->t, itemMap->index);
	}
	printf("------------------\n");
	for (i=0; i<LSLOG_MAX_LOG_NUM; i++) {
		fread(&item,sizeof(LogStorageItem), 1, logFile);
		printf("%d %s %s\n", (int)item.t, item.user, item.eventTpl);
	}
	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n");
}
#endif
