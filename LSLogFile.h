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

private:
	int searchBigerIndex(time_t key);
	int searchSmallerIndex(time_t key);
	unsigned getLogFileSize();

private:
	struct LogFileHeader {
		int currentIndex;
		bool loopCover;
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
