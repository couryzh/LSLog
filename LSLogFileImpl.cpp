#include "log.h"
#include "LSLogFile.h"
#include "LSLogMemPool.h"
#include "LSLogTemplate.h"
#include "LSLogCacheQueue.h"
#include "LSLogFileImpl.h"

LSLogFileImpl::LSLogFileImpl(LSLogMemPool *pool)
	: memPool(pool)
{
	int err;

	logTpl = new LSLogTemplate();
	for (int i=0; i<LOG_NUM; i++)
		logFile[i] = new LSLogFile(i, pool, logTpl);
	cacheQueue = new LSLogCacheQueue();

	if ((err = pthread_create(&saveThread, NULL, saveTaskThread, this)) != 0) {
		myLog("create saveTaskThread failed");
		exit(-1);
	}

	// start save
	threadRun = true;
}

LSLogFileImpl::~LSLogFileImpl()
{
	for (int i=0; i<LOG_NUM; i++) {
		delete logFile[i];
	}
	delete cacheQueue;
}


void * LSLogFileImpl::saveTaskThread(void *arg)
{
	LSLogFileImpl *th = static_cast<LSLogFileImpl*>(arg);
	th->threadRun = false;	
	th->saveLog();

	return NULL;
}

bool LSLogFileImpl::log(LogType type, time_t t, char user, const char *event)
{
	LSLogInfo *logInfo = memPool->malloc();		
	logInfo->type = type;
	logInfo->t = t;
	logInfo->user = user;
	strcpy(logInfo->event, event);

	myLog("in queue");
	cacheQueue->in(logInfo);

	return true;
}

int LSLogFileImpl::queryLog(time_t from, time_t to, int pageCapacity, int pageIndex, struct LSLogInfo *&logInfos)
{
	return 0;
}

void LSLogFileImpl::saveLog()
{
	while (1) {
		LSLogInfo *logInfo = cacheQueue->out();

		logFile[logInfo->type]->save(logInfo);	
		memPool->free(logInfo);
	}
}
