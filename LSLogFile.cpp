#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "log.h"
#include "LSLogMemPool.h"
#include "LSLogTemplate.h"
#include "LSLogFile.h"

//#define OLD_VERSION

#define LSLOG_INC(i)		((i + 1) % LSLOG_MAX_LOG_NUM)
#define LSLOG_DEC(i)		((i + LSLOG_MAX_LOG_NUM - 1) % LSLOG_MAX_LOG_NUM)

#define HDR_SIZE			sizeof(struct LogFileHeader)
#define ITEM_SIZE 			sizeof(LogStorageItem)
#define HDR_MAP_OFF(index)  sizeof(short) * 2 + sizeof(LogHeaderItem) * index
#define ITEM_OFF(index)		HDR_SIZE + sizeof(LogStorageItem) * index
#define INVALID_T			0

#define LSLOG_PERM 			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH

LSLogFile::LSLogFile(unsigned type, LSLogMemPool *pool, LSLogTemplate *tpl)
	: initSucc(false), logType(type), logTpl(tpl), memPool(pool)
{
	unsigned logFileSize;
	const char *fileName;
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
	}

	isNewFile = (access(logPath, F_OK) < 0) ? true : false;
	logFileFd = open(logPath, O_RDWR | O_CREAT, LSLOG_PERM);
	if (logFileFd < 0) {
		myLogErr("creat %s failed", logPath);
	}

	// 若为新建文件，设置文件大小
	logFileSize = getLogFileSize();
	if (isNewFile && ftruncate(logFileFd, logFileSize) < 0) {
		close(logFileFd);
		myLog("expand logfile size to %u", logFileSize);
	}

	pthread_rwlock_init(&rwlock, NULL);

	if (isNewFile) {
		fileHeader.minIndex = fileHeader.maxIndex = -1;
		for (int i=0; i<LSLOG_MAX_LOG_NUM; i++) {
			fileHeader.logHeaderTable[i].t = INVALID_T;
			fileHeader.logHeaderTable[i].offset = i;
		}
		dumpHeader();
	}
	else {
		if (!loadHeader()) {
			myLog("failed to laodHeader()");
		}
	}
	initSucc = true;
}

LSLogFile::~LSLogFile()
{
	close(logFileFd);

	pthread_rwlock_destroy(&rwlock);

	//myLog("destruct LSLogTemplate");
}

bool LSLogFile::loadHeader()
{
	unsigned loadSize;
	lseek(logFileFd, 0, SEEK_SET);

//	return read(logFileFd, &fileHeader, sizeof(LogFileHeader)) == sizeof(LogFileHeader);
	loadSize = read(logFileFd, &fileHeader, sizeof(LogFileHeader));
	myLog("load Header actually: %u, %u", loadSize, sizeof(LogFileHeader));
	return loadSize == sizeof(LogFileHeader);
}

bool LSLogFile::dumpHeader()
{
	unsigned dumpSize;
	lseek(logFileFd, 0, SEEK_SET);

	//return (write(logFileFd, &fileHeader, sizeof(LogFileHeader)) == sizeof(LogFileHeader));
	dumpSize = write(logFileFd, &fileHeader, sizeof(LogFileHeader));
	myLog("dump header actually: %u, %u", dumpSize, sizeof(LogFileHeader));
	return dumpSize == sizeof(LogFileHeader);
}

unsigned LSLogFile::getLogFileSize()
{
	return HDR_SIZE + LSLOG_MAX_LOG_NUM * ITEM_SIZE;
}

bool LSLogFile::save(LSLogInfo *logInfo)
{
	short nextIndex, newIndex;
	long itemOffset;
	LogStorageItem newLogItem;

	//myLog("save begin");

	if (!initSucc) return false;
	
	// 转成模板
	if (!logTpl->shrink(logInfo, &newLogItem)) {
		myLog("faile to convert template");
		return false;
	}

	pthread_rwlock_wrlock(&rwlock);
	if (newLogItem.t < fileHeader.logHeaderTable[fileHeader.maxIndex].t) {
		// 查找此时刻对应index
#if defined OLD_VERSION 
		newIndex = search(newLogItem.t);
#else
		newIndex = searchLeftBorder(newLogItem.t);
#endif
		myLog("find new insert index %hd for t=%d", newIndex, (int)newLogItem.t);

		// 保存信息
		itemOffset = ITEM_OFF(newIndex);
		lseek(logFileFd, itemOffset, SEEK_SET);
		write(logFileFd, &newLogItem, sizeof(LogStorageItem));

		// 更新文件头
		fileHeader.logHeaderTable[newIndex].t = (int)newLogItem.t;
		fileHeader.maxIndex = newIndex;
		myLog("update, maxIndex=%hd", fileHeader.maxIndex);
		dumpHeader();
	}
	else {
		// 更大的时标，直接添加到maxIndex后
		nextIndex = LSLOG_INC(fileHeader.maxIndex);
		itemOffset = ITEM_OFF(nextIndex);
		if (lseek(logFileFd, itemOffset, SEEK_SET) == (off_t)-1) {
			myLogErr("lseek");
		}
		else {
			write(logFileFd, &newLogItem, sizeof(LogStorageItem));
		}

		fileHeader.logHeaderTable[nextIndex].t = newLogItem.t;
		if (lseek(logFileFd, HDR_MAP_OFF(nextIndex), SEEK_SET) == (off_t)-1)
			myLogErr("lseek");
		else { 
			write(logFileFd, &fileHeader.logHeaderTable[nextIndex], sizeof(LogHeaderItem));
		}
		fileHeader.maxIndex = nextIndex;
		/* minIndex为初始值，这里maxIndex更新了，需要将minIndex更新 */
		if (fileHeader.minIndex == -1) {
			fileHeader.minIndex = 0;
			lseek(logFileFd, 0, SEEK_SET);
			write(logFileFd, &fileHeader.minIndex, sizeof(short));
		}
		/* maxIndex增加到开头再次追上minIndex，更新minIndex */
		else if (fileHeader.maxIndex == fileHeader.minIndex) {
			fileHeader.minIndex = LSLOG_INC(fileHeader.minIndex);
			lseek(logFileFd, 0, SEEK_SET);
			write(logFileFd, &fileHeader.minIndex, sizeof(short));
		}

		lseek(logFileFd, sizeof(short), SEEK_SET);
		write(logFileFd, &fileHeader.maxIndex, sizeof(short));

		myLog("minIndex=%hd maxIndex=%hd", fileHeader.minIndex, fileHeader.maxIndex);
	}
	pthread_rwlock_unlock(&rwlock);

	//myLog("saved finish");
	return true;
}

int LSLogFile::query(time_t from, time_t to, int blockSize, int blockIndex, struct LSLogInfo *& logInfos)
{
	int i, offIndex, index, totalInfo;
	short beginIndex, endIndex;
	long offset;
	LogStorageItem item;
	LSLogInfo *logInfo, *prev;

	// 无法提供服务，返回
	if (!initSucc) return 0;
	
	prev = NULL;
	pthread_rwlock_rdlock(&rwlock);
	logInfos = NULL;

	//myLog("from %d to %d", (int)from, (int)to);
	if (!searchRange(from, to, beginIndex, endIndex)) { 
		// 无对应时间段数据
		return 0;
	}
	myLog("beginIndex=%d, endIndex=%d", beginIndex, endIndex);

	// 开始，结束之间的长度，这里代表查询到的总条数
	if (endIndex >= beginIndex) {
		totalInfo = (endIndex - beginIndex) + 1;
	}
	else {
		totalInfo = endIndex + LSLOG_MAX_LOG_NUM - beginIndex + 1;
	}

	// 得到指定的记录
	offIndex = (blockIndex - 1) * blockSize;
	for (i=0; i<blockSize; i++) {
		// 防止查询越界
		if (offIndex + i >= totalInfo) break;

		index = (beginIndex + offIndex + i) % LSLOG_MAX_LOG_NUM; 

		offset = ITEM_OFF(index);
		lseek(logFileFd, offset, SEEK_SET);
		if (read(logFileFd, &item, sizeof(LogStorageItem)) != sizeof(LogStorageItem)) {
			myLog("failed to get LogStorageItem");
			continue;
		}
		logInfo = memPool->malloc();	
		// 扩展模板
		if (!logTpl->expand(&item, logInfo)) {
			myLog("failed to expand %s", item.eventTpl);
			memPool->free(logInfo);
			continue;
		}
		//myLog("expand: %s -> %s", item.eventTpl, logInfo->event);
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

bool LSLogFile::searchRange(time_t from, time_t to, short &left, short &right)
{
#if defined OLD_VERSION
	short i;
#endif

	//myLog("searchRange(%d,%d)  minIndex(%hd): %d maxIndex(%hd): %d\n", (int)from, (int)to, 
	//		fileHeader.minIndex, fileHeader.minIndex==-1 ? 0 : fileHeader.logHeaderTable[fileHeader.minIndex].t, 
	//		fileHeader.maxIndex, fileHeader.maxIndex==-1 ? 0x7fffffff : fileHeader.logHeaderTable[fileHeader.maxIndex].t);

	// 没有任何日志信息
	if (fileHeader.minIndex == -1 || fileHeader.maxIndex == -1) {
		left = right = -1;
		return false;
	}

	// 非法时间
	if (from > to) {
		left = right = -1;
		return false;
	}
	// 在已保存时间之外，则定位范围失败
	if ((int)from > fileHeader.logHeaderTable[fileHeader.maxIndex].t
			|| (int)to < fileHeader.logHeaderTable[fileHeader.minIndex].t) {
		myLog("time overrange now between(%d, %d)",
				fileHeader.logHeaderTable[fileHeader.minIndex].t,
				fileHeader.logHeaderTable[fileHeader.maxIndex].t);
		left = right = -1;
		return false;
	}

	// 增序遍历，查找左边界
#if defined OLD_VERSION
	for (i=fileHeader.minIndex; i!=fileHeader.maxIndex; i=LSLOG_INC(i)) {
		if (fileHeader.logHeaderTable[i].t >= (int)from)
			break;
	}
	left = i;
#else
	left = searchLeftBorder((int)from);
#endif

	// 降序遍历，查找右边界
#if defined OLD_VERSION
	for (i=fileHeader.maxIndex; i!=fileHeader.minIndex; i=LSLOG_DEC(i)) {
		if (fileHeader.logHeaderTable[i].t <= (int)to)
			break;
	}
	right = i;
#else
	right = searchRightBorder((int)to);
#endif

	return true;
}

// 找到t不小于key的索引
short LSLogFile::search(time_t key)
{
	short i;

	if (fileHeader.logHeaderTable[fileHeader.maxIndex].t < (int)key) {
		return fileHeader.maxIndex;
	}

	for (i=fileHeader.minIndex; i!=fileHeader.maxIndex; i=LSLOG_INC(i)) {
		if (fileHeader.logHeaderTable[i].t > (int)key)
			break;
	}
	return i;
}

short LSLogFile::searchBetween(short low, short high, int key, int cond)
{
	short middle, match;
	int shortEnough;

	//printf("search %d in array %s\n", key, printIntArr(nums, len, buf));

	middle = (low + high) / 2;
	match = -1;
	shortEnough = 0;

	while (!shortEnough) {
		printf("delta length %d (%d, %d)\n", high - low + 1, fileHeader.logHeaderTable[low].t, fileHeader.logHeaderTable[high].t);
		if (high - low >= 3) {
			if (fileHeader.logHeaderTable[middle].t > key) {
				high = middle;
			}
			else if (fileHeader.logHeaderTable[middle].t < key) {
				low = middle;
			}
			else {
				if (cond == CMP_GE) {
					match = middle;
				}
				else if (cond == CMP_LE) {
					match = middle;

				}
				break;
			}
			middle = (low + high) / 2;
		}
		else {
			int tmp;
			shortEnough = 1;
			if (cond == CMP_GE) {
				for (tmp = low; tmp <=high; tmp++) {
					if (fileHeader.logHeaderTable[tmp].t > key) {
						match = tmp;
						break;
					}
				}
				if (match == -1 && fileHeader.logHeaderTable[high].t == key)
					match = high;
			}
			else if (cond == CMP_LE) {
				for (tmp = high; tmp >=low; tmp--) {
					if (fileHeader.logHeaderTable[tmp].t < key) {
						match = tmp;
						break;
					}
				}
				if (match == -1 && fileHeader.logHeaderTable[low].t == key)
					match = low;
			}
		}
	}

	printf("[%d]: %d\n", match, fileHeader.logHeaderTable[match].t);
	return match;
}

short LSLogFile::searchLeftBorder(time_t key)
{
	short low, high;

	if (fileHeader.logHeaderTable[fileHeader.maxIndex].t < (int)key) {
		return fileHeader.maxIndex;
	}

	if (fileHeader.minIndex > fileHeader.maxIndex) {
		if (fileHeader.logHeaderTable[LSLOG_MAX_LOG_NUM - 1].t <= (int)key) {
			low = 0;
			high = fileHeader.maxIndex;
		}
		else {
			low = fileHeader.minIndex;
			high = LSLOG_MAX_LOG_NUM-1;
		}
		return searchBetween(low, high, (int)key, CMP_GE);
	}
	return searchBetween(fileHeader.minIndex, fileHeader.maxIndex, (int)key, CMP_GE);
}


short LSLogFile::searchRightBorder(time_t key)
{
	short low, high;

	if (fileHeader.logHeaderTable[fileHeader.minIndex].t > (int)key) {
		return fileHeader.minIndex;
	}

	if (fileHeader.minIndex > fileHeader.maxIndex) {
		if (fileHeader.logHeaderTable[LSLOG_MAX_LOG_NUM - 1].t <= (int)key) {
			low = 0;
			high = fileHeader.maxIndex;
		}
		else {
			low = fileHeader.minIndex;
			high = LSLOG_MAX_LOG_NUM-1;
		}
		return searchBetween(low, high, (int)key, CMP_LE);
	}
	return searchBetween(fileHeader.minIndex, fileHeader.maxIndex, (int)key, CMP_LE);
}

void LSLogFile::printHeader()
{
	int i;
	LogHeaderItem *headerItem;

	printf("%d ----------------------------\n"
		   "minIndex: %hd maxIndex: %hd\n", logType, fileHeader.minIndex, fileHeader.maxIndex);
	for (i=0; i<LSLOG_MAX_LOG_NUM; i++) {
		headerItem = &fileHeader.logHeaderTable[i];
		printf("t: %d index: %hd\n", 
				headerItem->t, headerItem->offset);
	}
	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n");
}

