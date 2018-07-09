#ifndef LSLOGFILE_H 
#define LSLOGFILE_H 

#include "LSLogCommon.h"

class LSLogTemplate;
class LSLogMemPool;


// 一条实际存储的日志记录
struct LogStorageItem {
	time_t t;
	char eventTpl[LSLOG_MAX_EVENT_TPL_LEN+1];
};

class LSLogFile {
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
	bool initSucc;
	unsigned logType;
	int logFileFd;
	LogFileHeader fileHeader;
	char logPath[LSLOG_MAX_PATH_LEN+1];
	pthread_rwlock_t rwlock;

	LSLogTemplate *logTpl;	
	LSLogMemPool *memPool;
};

#endif
