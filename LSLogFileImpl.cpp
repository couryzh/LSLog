#include "log.h"
#include "LSLogFile.h"
#include "LSLogMemPool.h"
#include "LSLogTemplate.h"
#include "LSLogCacheQueue.h"
#include "LSLogFileImpl.h"

#include <sys/time.h>

LSLogFileImpl::LSLogFileImpl(LSLogMemPool *pool)
	: initSucc(true), memPool(pool)
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
		initSucc = false;
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

bool LSLogFileImpl::log(LogType type, time_t t, char *event)
{
	if (!initSucc) return false;

	LSLogInfo *logInfo = NULL; 
	if (strlen(event) > LSLOG_MAX_EVENT_LEN ) {
		myLog("too long event string");
		return false;
	}

	logInfo = memPool->malloc();		
	logInfo->type = type;
	logInfo->t = t;
	if (event==NULL || strlen(event) > LSLOG_MAX_EVENT_TPL_LEN) {
		myLog("event too long");
		goto FAIL;
	}
	strcpy(logInfo->event, event);
	logInfo->isTpl = true;

	//myLog("in queue: t=%d", (int)t);
	// 缓存到队列，如果有消费者等待则唤醒它
	cacheQueue->in(logInfo);

	return true;
FAIL:
	if (logInfo)
		memPool->free(logInfo);
	return false;
}


bool LSLogFileImpl::log(LogType type, char *event)
{
	time_t t;

	t = time(NULL);
	return log(type, t, event);		
}

int LSLogFileImpl::queryLog(int type, time_t from, time_t to, int pageCapacity, int pageIndex, struct LSLogInfo *&logInfos)
{
	if (!initSucc) return 0;
	if (type < LOG_NUM) {
		return logFile[type]->query(from, to, pageCapacity, pageIndex, logInfos);
	}
	return 0;
}

void LSLogFileImpl::saveLog()
{
	while (threadRun) {
		//myLog("savelog run...");
		// 取出缓存队列的元素, 队列空时阻塞
		LSLogInfo *logInfo = cacheQueue->out();

		logFile[logInfo->type]->save(logInfo);	
		memPool->free(logInfo);
	}
}
