#include "log.h"
#include "LSLogMemPool.h"

LSLogMemPool::LSLogMemPool(int initSize)
	: size(0), freeList(NULL) 
{
	int poolSize;
	
	poolSize = LSLOG_MEMPOOL_BLOCK_SIZE;	
	for (; poolSize < initSize; poolSize <<= 1);
		
	allocUnlock(poolSize);

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);
}

LSLogMemPool::~LSLogMemPool()
{
	pthread_mutex_lock(&lock);
	LSLogInfo *ptr, *tmp;
	for (ptr = freeList; ptr; ptr=tmp) {
		tmp = ptr->next;
		delete ptr;
	}
	size = 0;
	pthread_mutex_unlock(&lock);

	pthread_mutexattr_destroy(&attr);
	pthread_mutex_destroy(&lock);
}

void LSLogMemPool::allocUnlock(int n)
{
	LSLogInfo *p;

	for (int i=0; i<n; i++) {
		p = new LSLogInfo();
		free(p);
	}
	size += n;
}

LSLogInfo *LSLogMemPool::malloc()
{
	pthread_mutex_lock(&lock);
	if (size == 0) {
		allocUnlock();
	}

	LSLogInfo *logInfo;
	logInfo = freeList;
	freeList = freeList->next;
	size -= 1;
	logInfo->next = NULL;
	pthread_mutex_unlock(&lock);
	return logInfo;
}

LSLogInfo *LSLogMemPool::malloc(int n)
{
	LSLogInfo *head, *curr, *prev;

	head = prev = NULL;
	pthread_mutex_lock(&lock);
	for (int i=0; i<n; i++) {
		curr = malloc();	
		curr->next = NULL;
		if (prev == NULL) 
			prev = curr;
		else	
			prev->next = curr;

		if (head == NULL)
			head = curr;
	}
	pthread_mutex_unlock(&lock);
	return head;
}

void LSLogMemPool::free(void *p)
{
	pthread_mutex_lock(&lock);
	LSLogInfo *logInfo = (LSLogInfo *)p;
	if (freeList) {
		logInfo->next = freeList;
		freeList = logInfo;
	}
	else {
		freeList = logInfo;
		logInfo->next = NULL;
	}

	pthread_mutex_unlock(&lock);
}


void LSLogMemPool::dump()
{
	LSLogInfo *p;
	printf("mempool freelist\n");
	for (p=freeList; p; p=p->next) {
		printf("%p ", p);
	}
	printf("\n");
}
