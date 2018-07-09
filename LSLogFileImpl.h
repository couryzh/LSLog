#ifndef LSLOGFILEIMPL_H
#define LSLOGFILEIMPL_H

#include <pthread.h>
#include "LSLogCommon.h"

class LSLogFile;
class LSLogTemplate;
class LSLogMemPool;
class LSLogCacheQueue;

class LSLogFileImpl {
public:
	LSLogFileImpl(LSLogMemPool *pool);
	~LSLogFileImpl();

	bool log(LogType type, time_t t, char *event);
	bool log(LogType type, char *event);
	int queryLog(int type, time_t from, time_t to, 
			int pageCapacity, int pageIndex, 
			struct LSLogInfo *&logInfos);
	static  void * saveTaskThread(void *arg);


private:
	void saveLog();
	void stopSave();

private:
	bool initSucc;
	bool threadRun;
	pthread_t saveThread;
	LSLogCacheQueue *cacheQueue;
	LSLogMemPool *memPool;
	LSLogTemplate *logTpl;
	LSLogFile *logFile[LOG_NUM];
};

#endif
