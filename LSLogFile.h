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
	void dump();
#endif

private:
	int searchLeft(time_t key);
	int searchRight(time_t key);
	void unset(int fromIndex, int toIndex);
	unsigned getLogFileSize();
	void dumpStorageItem(LogStorageItem *item);

private:
	struct LogFileHeader {
		int headIndex;
		int tailIndex;
	};
	
	unsigned logType;
	bool isNewFile;
	int  logFileFd;
	void *mapAddr;
	LogFileHeader fileHeader;
	char logPath[LSLOG_MAX_PATH_LEN];
	pthread_rwlock_t rwlock;

	LSLogTemplate *logTpl;	
	LSLogMemPool *memPool;
};

#endif
