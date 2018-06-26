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
	LogStorageItem * searchIndex(time_t key);

private:
	struct LogFileHeader {
		char overed; 		// 已写满，从头开始写
		int currentIndex;
	};
	
	LogFileHeader fileHeader;
	bool isCreateFile;

	int  logFileFd;
	void *mapAddr;
	char logPath[LSLOG_MAX_PATH_LEN];

	LSLogTemplate *logTpl;	
	LSLogMemPool *memPool;


};

#endif
