#ifndef LSLOGFILEIMPL_H
#define LSLOGFILEIMPL_H

#include <pthread.h>
#include "LSLog.h"

class LSLogFile;
class LSLogTemplate;
class LSLogMemPool;
class LSLogCacheQueue;

class LSLogFileImpl : public LSLog {
public:
	enum {defaultMemPoolSize = 32};
	LSLogFileImpl(LSLogMemPool *pool);
	~LSLogFileImpl();

	bool log(LogType type, time_t t, char user, const char *event);
	int queryLog(time_t from, time_t to, int pageCapacity,
			int pageIndex, struct LSLogInfo *&logInfos);
	static  void * saveTaskThread(void *arg);

private:
	void saveLog();

private:
	bool threadRun;
	pthread_t saveThread;
	LSLogCacheQueue *cacheQueue;
	LSLogFile *logFile[LOG_NUM];
	LSLogMemPool *memPool;
	LSLogTemplate *logTpl;
};

#endif
