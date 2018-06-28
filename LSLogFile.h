#ifndef LSLOGFILE_H 
#define LSLOGFILE_H 

#include "LSLogCommon.h"

class LSLogTemplate;
class LSLogMemPool;

class LSLogFile {
	friend class LSLogTemplate;
public:
	LSLogFile(unsigned type, LSLogMemPool *pool, LSLogTemplate *tpl);
	~LSLogFile();
	bool save(LSLogInfo *logInfo);

	int query(time_t from, time_t to, int blockSize, int blockIndex, struct LSLogInfo *& logInfos);

#if 1
	void print();
#endif

private:
	short searchLeft(time_t key);
	short searchRight(time_t key);
	void unset(int fromIndex, int toIndex);

private:
	short search(time_t key);
	unsigned getLogFileSize();
	void printStorageItem(LogStorageItem *item);
	bool loadHeader();
	bool dumpHeader();

private:
#pragma pack(2)	
	struct LogItemMap {
		int t;
		short index;
	};
	struct LogFileHeader {
		short nextMapPos;
		LogItemMap logItemMap[LSLOG_MAX_LOG_NUM];
	};
#pragma pack()	

	unsigned logType;
	FILE *logFile;
	LogItemMap *auxItemMap;
	LogFileHeader fileHeader;
	char logPath[LSLOG_MAX_PATH_LEN+1];
	pthread_rwlock_t rwlock;

	LSLogTemplate *logTpl;	
	LSLogMemPool *memPool;
};

#endif
