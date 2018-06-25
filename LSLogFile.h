#ifndef LSLOGFILE_H 
#define LSLOGFILE_H 

#include "LSLogCommon.h"

#define LSLOG_FILE 		"ls.log"

class LSLogTemplate;

class LSLogFile {
	friend class LSLogTemplate;
public:
	LSLogFile();
	~LSLogFile();
	bool save(LSLogInfo *logInfo);

	int query(time_t from, time_t to, int blockSize, int blockIndex, struct LSLogInfo *& logInfos);

private:
	LogStorageItem * searchIndex(time_t key);

private:
	struct LogFileHeader {
		int overed; 		// 已写满，从头开始写
		//short itemSize;
		int currentIndex;
	};
	
	char logPath[128];
	int  logFile;
	void *mapAddr;
	LSLogTemplate *logtpl;	
};

#endif
