#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "log.h"
#include "LSLogMemPool.h"
#include "LSLogTemplate.h"
#include "LSLogFile.h"

#define LSLOG_INC(i)		((i + 1) % LSLOG_MAX_LOG_NUM)
#define LSLOG_DEC(i)		((i + LSLOG_MAX_LOG_NUM - 1) % LSLOG_MAX_LOG_NUM)

#define HDR_SIZE			sizeof(struct LogFileHeader)
#define ITEM_SIZE 			sizeof(LogStorageItem)
#define HDR_MAP_OFF(index)  sizeof(short) + sizeof(LogHeaderItem) * index
#define ITEM_OFF(index)		HDR_SIZE + sizeof(LogStorageItem) * index
#define LAST_INDEX(hdr)     LSLOG_DEC(hdr.nextIndex)

#define INVALID_T			0 //0x7fffffff

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
	logFileFd = open(logPath, O_RDWR | O_CREAT, LSLOG_PERM);
	if (logFileFd < 0) {
		myLogErr("creat %s failed", logPath);
		exit(-1);
	}
	logFile = fdopen(logFileFd, "rb+");
	if (!logFile) {
		myLogErr("open %s for log failed", logPath);
		exit(-1);
	}

	logFileSize = getLogFileSize();
	logFileFd = fileno(logFile);
	if (isNewFile && ftruncate(logFileFd, logFileSize) < 0) {
		fclose(logFile);
		myLog("expand logfile size to %u", logFileSize);
		exit(-1);
	}

	pthread_rwlock_init(&rwlock, NULL);

	if (isNewFile) {
		int i;
		myLog("isNewFile=true");
		fileHeader.nextIndex= 0;
		for (i=0; i<LSLOG_MAX_LOG_NUM; i++) {
			fileHeader.logHeaderTable[i].t = INVALID_T;
			fileHeader.logHeaderTable[i].offset = i;
		}
		dumpHeader();
	}
	else {
		myLog("loadHeader()");
		if (!loadHeader()) {
			myLog("failed to laodHeader()");
			exit(-1);
		}
	}

	auxHeaderTable= new LogHeaderItem[LSLOG_MAX_LOG_NUM/2 + 1];
	if (auxHeaderTable == NULL) {
		myLog("alloc auxHeaderTable failed");
		exit(-1);
	}
}

LSLogFile::~LSLogFile()
{
	fclose(logFile);

	pthread_rwlock_destroy(&rwlock);

	//myLog("destruct LSLogTemplate");
}

void LSLogFile::sortHeaderTable(int(*cmp)(const void *, const void *))
{
	qsort(fileHeader.logHeaderTable, LSLOG_MAX_LOG_NUM, sizeof(LogHeaderItem), cmp);
}

short LSLogFile::searchMapIndex(short index)
{
	void *matchPtr;
	LogHeaderItem headerItem;

	headerItem.t = 0;
	headerItem.offset = index;
	
	matchPtr = bsearch(&headerItem, fileHeader.logHeaderTable, LSLOG_MAX_LOG_NUM, sizeof(LogHeaderItem), cmpOffset);
	assert(matchPtr != NULL);
	return ((LogHeaderItem *)matchPtr - fileHeader.logHeaderTable) / sizeof(LogHeaderItem) + 1;
}


bool LSLogFile::loadHeader()
{
	fseek(logFile, 0, SEEK_SET);
	if (fread(&fileHeader.nextIndex, sizeof(short), 1, logFile) < 1) {
		perror("fread");
		return  false;
	}
	for (int i=0; i<LSLOG_MAX_LOG_NUM; i++) {
		if (fread(&fileHeader.logHeaderTable[i], sizeof(LogHeaderItem), 1, logFile) < 1) {
			perror("fread");
			return  false;
		}
	}

	sortHeaderTable(cmpOffset); // 已保证存入时是排序的， 这里有必要吗?
	return true;
}

bool LSLogFile::dumpHeader()
{
	fseek(logFile, 0, SEEK_SET);
	if (fwrite(&fileHeader, sizeof(LogFileHeader), 1,logFile) == 1) {
		fflush(logFile);
		return true;
	}
	else 
		return false;
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
	short nextIndex, newIndex, lastIndex;
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

	nextIndex = fileHeader.nextIndex;
	lastIndex = LAST_INDEX(fileHeader);
	myLog("nextIndex: %hd,  lastIndex: %hd", nextIndex, lastIndex);
	if (newLogItem.t < fileHeader.logHeaderTable[lastIndex].t) {
		myLog("insert older time");
		newIndex = searchLeft(newLogItem.t);

		// unset newer time
		sortHeaderTable(cmpTime);
		short  timeIndex = searchMapIndex(newIndex);
		myLog("search %d: return %hd", (int)newLogItem.t, timeIndex);
		for (short i=timeIndex; i<LSLOG_MAX_LOG_NUM; i++) {
			if (fileHeader.logHeaderTable[i].t == INVALID_T)
				break;
			fileHeader.logHeaderTable[i].t = INVALID_T;
		}
		myLog("after unset ");
		print();

		sortHeaderTable(cmpOffset);
		fileHeader.logHeaderTable[newIndex].t = (int)newLogItem.t;
		fileHeader.nextIndex = LSLOG_INC(timeIndex);
		myLog("update, nextIndex=%hd", fileHeader.nextIndex);
		if (fseek(logFile, 0, SEEK_SET) == 0)
			fwrite(&fileHeader.nextIndex, sizeof(short), 1, logFile);
		else
			myLog("fseek to SEEK_SET:0 failed");

		itemOffset = ITEM_OFF(newIndex);
		fseek(logFile, itemOffset, SEEK_SET);
		fwrite(&newLogItem, sizeof(LogStorageItem), 1, logFile);
		dumpHeader();
	}
	else {
		itemOffset = ITEM_OFF(nextIndex);
		fseek(logFile, itemOffset, SEEK_SET);
		fwrite(&newLogItem, sizeof(LogStorageItem), 1, logFile);

		fileHeader.logHeaderTable[nextIndex].t = newLogItem.t;
		fileHeader.nextIndex = LSLOG_INC(fileHeader.nextIndex); 
		fseek(logFile, 0, SEEK_SET);
		fwrite(&fileHeader.nextIndex, sizeof(short), 1, logFile);
		fseek(logFile, HDR_MAP_OFF(nextIndex), SEEK_SET);
		fwrite(&fileHeader.logHeaderTable[fileHeader.nextIndex], sizeof(LogHeaderItem), 1, logFile);
		myLog("append t=%d, now nextIndex=%hd", (int)newLogItem.t, fileHeader.nextIndex);
	}
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

	myLog("searchLeft %d", (int)key);
	print();
	if (fileHeader.logHeaderTable[fileHeader.nextIndex].t == INVALID_T) {
		headIndex = 0;
		tailIndex = LAST_INDEX(fileHeader);
	}
	else {
		headIndex = fileHeader.nextIndex;
		tailIndex = LAST_INDEX(fileHeader);
	}
	myLog("headIndex: %hd tailIndex: %hd", headIndex, tailIndex);
	if (fileHeader.logHeaderTable[tailIndex].t < (int)key) {
		myLog("tail: t=%d", (int)fileHeader.logHeaderTable[tailIndex].t);
		return tailIndex;
	}

	for (i=headIndex; i!=tailIndex; i=LSLOG_INC(i)) {
		if (fileHeader.logHeaderTable[i].t > (int)key)
			break;
	}
	return i;
}

short LSLogFile::searchRight(time_t key)
{
	short i, headIndex, tailIndex;

	if (fileHeader.logHeaderTable[fileHeader.nextIndex].t == INVALID_T) {
		headIndex = 0;
		tailIndex = LAST_INDEX(fileHeader);
	}
	else {
		headIndex = fileHeader.nextIndex;
		tailIndex = LAST_INDEX(fileHeader);
	}

	if (fileHeader.logHeaderTable[headIndex].t > (int)key) {
		return headIndex;
	}
	for (i=tailIndex; i!=headIndex; i=LSLOG_DEC(i)) { 
		if (fileHeader.logHeaderTable[i].t <= (int)key)
			break;
	}

	return i;
}

#if 1
void LSLogFile::print()
{
	int i;
	LogHeaderItem *headerItem;
	//LogStorageItem  item;
	LogFileHeader header;
	
	fseek(logFile, 0, SEEK_SET);
	if (fread(&header, sizeof(LogFileHeader), 1, logFile) != 1) {
		printf("print() read header failed\n");
	}

	printf("%d ----------------------------\n"
		   "nextIndex: %d\n", logType, header.nextIndex);
	for (i=0; i<LSLOG_MAX_LOG_NUM; i++) {
		headerItem = &fileHeader.logHeaderTable[i];
		printf("t: %d index: %hd\n", 
				headerItem->t, headerItem->offset);
	}
	//printf("------------------\n");
	//for (i=0; i<LSLOG_MAX_LOG_NUM; i++) {
	//	fread(&item,sizeof(LogStorageItem), 1, logFile);
	//	printf("%d %s %s\n", (int)item.t, item.user, item.eventTpl);
	//}
	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n");
}
#endif


int cmpOffset(const void *a, const void *b)
{
	LSLogFile::LogHeaderItem *itemA, *itemB;
	itemA = (LSLogFile::LogHeaderItem *)a;
	itemB = (LSLogFile::LogHeaderItem *)b;

	return itemA->offset - itemB->offset;
}

int cmpTime(const void *a, const void *b)
{
	LSLogFile::LogHeaderItem *itemA, *itemB;
	itemA = (LSLogFile::LogHeaderItem *)a;
	itemB = (LSLogFile::LogHeaderItem *)b;

	return itemA->t - itemB->t;
}
