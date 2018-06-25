#ifndef LSLOGMEMPOOL_H
#define LSLOGMEMPOOL_H

#include <pthread.h>
#include "LSLogCommon.h"

class LSLogMemPool {
public:
	LSLogMemPool(int initSize);
	~LSLogMemPool();

	LSLogInfo *alloc(void);
	LSLogInfo *alloc(int n);
	void free(void *);

private:
	enum {blockSize = 10};
	typedef LSLogInfo LogBlock[blockSize];

	pthread_mutex_t *lock;
	std::list<LSLogInfo *> freeList;
	std::list<LogBlock> allocedBlockList;
};

#endif
