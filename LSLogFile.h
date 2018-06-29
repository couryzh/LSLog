#ifndef LSLOGFILE_H 
#define LSLOGFILE_H 

#include "LSLogCommon.h"

class LSLogTemplate;
class LSLogMemPool;

class LSLogFile {
	friend class LSLogTemplate;
	friend int cmpOffset(const void * a,  const void *b);
	friend int cmpTime(const void *a, const void *b);
public:
	LSLogFile(unsigned type, LSLogMemPool *pool, LSLogTemplate *tpl);
	~LSLogFile();
	bool save(LSLogInfo *logInfo);

	int query(time_t from, time_t to, int blockSize, int blockIndex, struct LSLogInfo *& logInfos);

#if 1
	void printFile();
#endif

private:
	void unset(int fromIndex, int toIndex);

private:
	short search(time_t key);
	bool searchRange(time_t from, time_t to, short &left, short &right);
	void sortHeaderTable(int(*cmp)(const void *, const void *));
	short searchMapIndex(short index);
	unsigned getLogFileSize();
	void printStorageItem(LogStorageItem *item);
	bool loadHeader();
	bool dumpHeader();
	void printHeader();

private:
#pragma pack(2)	
	struct LogHeaderItem{
		int t;
		short offset;
	};
	struct LogFileHeader {
		short nextIndex;
		LogHeaderItem logHeaderTable[LSLOG_MAX_LOG_NUM];
	};
#pragma pack()	

	unsigned logType;
	int logFileFd;
	LogHeaderItem *auxHeaderTable;
	LogFileHeader fileHeader;
	short minIndex, maxIndex;
	char logPath[LSLOG_MAX_PATH_LEN+1];
	pthread_rwlock_t rwlock;

	LSLogTemplate *logTpl;	
	LSLogMemPool *memPool;
};


int cmpOffset(const void * a,  const void *b);
int cmpTime(const void * a,  const void *b);

#endif
