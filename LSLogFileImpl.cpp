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
	for (int i=0; i<LOG_NUM; i++) {
		logFile[i] = new LSLogFile(i, pool, logTpl);
	}
	cacheQueue = new LSLogCacheQueue();

	threadRun = true;
	if ((err = pthread_create(&saveThread, NULL, saveTaskThread, this)) != 0) {
		myLog("create saveTaskThread failed");
		exit(-1);
	}
}

LSLogFileImpl::~LSLogFileImpl()
{
	stopSave();
	for (int i=0; i<LOG_NUM; i++) {
		delete logFile[i];
	}
	delete cacheQueue;
}

void LSLogFileImpl::stopSave()
{
	threadRun = false;
	pthread_cancel(saveThread);
}

void * LSLogFileImpl::saveTaskThread(void *arg)
{
	LSLogFileImpl *th = static_cast<LSLogFileImpl*>(arg);
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

<<<<<<< HEAD
=======
	myLog("in queue");
>>>>>>> 396e9f98478b6724a72beb0172ab39520266f8e1
	// 缓存到队列，如果有消费者等待则唤醒它
	cacheQueue->in(logInfo);

	return true;
}

int LSLogFileImpl::queryLog(LogType type, time_t from, time_t to, int pageCapacity, int pageIndex, struct LSLogInfo *&logInfos)
{
	if (type < LOG_NUM) {
		return logFile[type]->query(from, to, pageCapacity, pageIndex, logInfos);
	}
	return 0;
}

void LSLogFileImpl::saveLog()
{
	while (threadRun) {
<<<<<<< HEAD
		myLog("savelog run...");
=======
>>>>>>> 396e9f98478b6724a72beb0172ab39520266f8e1
		// 取出缓存队列的元素, 队列空时阻塞
		LSLogInfo *logInfo = cacheQueue->out();

		logFile[logInfo->type]->save(logInfo);	
		memPool->free(logInfo);
	}
}
