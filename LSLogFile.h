#ifndef LSLOGFILE_H 
#define LSLOGFILE_H 

#include "LSLogCommon.h"

class LSLogTemplate;
class LSLogMemPool;


// 一条实际存储的日志记录
struct LogStorageItem {
	time_t t;
	char user[LSLOG_MAX_USER_TPL_LEN+1];
	char eventTpl[LSLOG_MAX_EVENT_TPL_LEN+1];
};

class LSLogFile {
#if 0
	friend int cmpOffset(const void * a,  const void *b);
	friend int cmpTime(const void *a, const void *b);
#endif

public:
	LSLogFile(unsigned type, LSLogMemPool *pool, LSLogTemplate *tpl);
	~LSLogFile();
	bool save(LSLogInfo *logInfo);

	int query(time_t from, time_t to, int blockSize, int blockIndex, struct LSLogInfo *& logInfos);

private:
	bool loadHeader();
	bool dumpHeader();
	short search(time_t key);
	bool searchRange(time_t from, time_t to, short &left, short &right);
	unsigned getLogFileSize();

	void printHeader();
	//void sortHeaderTable(int(*cmp)(const void *, const void *));

private:
#pragma pack(2)	
	struct LogHeaderItem{
		int t;
		short offset;
	};
	struct LogFileHeader {
		short minIndex, maxIndex;
		LogHeaderItem logHeaderTable[LSLOG_MAX_LOG_NUM];
	};
#pragma pack()	

	unsigned logType;
	int logFileFd;
	LogFileHeader fileHeader;
	char logPath[LSLOG_MAX_PATH_LEN+1];
	pthread_rwlock_t rwlock;

	LSLogTemplate *logTpl;	
	LSLogMemPool *memPool;
};

#if 0
int cmpOffset(const void * a,  const void *b);
int cmpTime(const void * a,  const void *b);
#endif

#endif
