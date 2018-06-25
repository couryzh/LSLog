#ifndef LSLOGMEMPOOL_H
#define LSLOGMEMPOOL_H

#include <pthread.h>
#include "LSLogCommon.h"

#define LSLOG_MEMPOOL_BLOCK_SIZE	8

class LSLogMemPool {
public:
	LSLogMemPool(int initSize);
	~LSLogMemPool();

	LSLogInfo *malloc();
	LSLogInfo *malloc(int n);
	void free(void *);

private:
	void allocUnlock(int n=LSLOG_MEMPOOL_BLOCK_SIZE);

private:
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
	
	// 可用数量
	int size;

	// 空闲链表
	LSLogInfo *freeList;
};

#endif
