#include "LSLogCacheQueue.h"

LSLogCacheQueue::LSLogCacheQueue()
{
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&cond, NULL);
}
LSLogCacheQueue::~LSLogCacheQueue()
{
	queue.clear();
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&cond);
}

void LSLogCacheQueue::in(LSLogInfo *logInfo)
{
	pthread_mutex_lock(&lock);
	
	std::list<LSLogInfo *>::iterator it = queue.begin();
	while (it != queue.end()) {
		if ((*it)->t > logInfo->t)
			break;
		++it;
	}
	queue.insert(it, logInfo);

	if (isQueueEmpty) {
		isQueueEmpty = false;
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&lock);
}

LSLogInfo *LSLogCacheQueue::out()
{
	LSLogInfo *logInfo;

	pthread_mutex_lock(&lock);
	while (queue.size() == 0) {
		isQueueEmpty = true;
		pthread_cond_wait(&cond, &lock);
	}

	logInfo = queue.front();
	queue.pop_front();
	pthread_mutex_unlock(&lock);
	return logInfo;
}

