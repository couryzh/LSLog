#ifndef LSLOGCACHEQUEUE_H
#define LSLOGCACHEQUEUE_H

#include <pthread.h>
#include "LSLogCommon.h"

class LSLogCacheQueue {
public:
	LSLogCacheQueue();
	void in(LSLogInfo *logInfo);
	LSLogInfo *out();

private:
	std::list<LSLogInfo *> queue;
	pthread_mutex_t *lock;
	pthread_cond_t *isEmpty;

};

#endif
