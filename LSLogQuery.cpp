#include "LSLogMemPool.h"
#include "LSLogFileImpl.h"
//#include "LSLogFile.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

// ---------------------------------------
LSLogMemPool *pool;
LSLogFileImpl *log;

void init()
{
	pool = new LSLogMemPool(10);
	log = new LSLogFileImpl(pool);
}

void destroy()
{
	delete pool;
	delete log;
}
void query(int from, int to, int cap, int index)
{
	int total;
	LSLogInfo *logInfo, *p;

	//log->logFile[0]->printHeader();
	
	total = log->queryLog(OPE_LOG, from, to, cap, index, logInfo);
	printf("query %d-%d\n", from, to);
	printf("total: %d\n", total);
	for (p=logInfo; p != NULL; p=p->next) {
		printf("%5d %s\n", (int)(p->t), p->event);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	int i, from, to;
	int pageCap, pageIndex;
	time_t t;

	//if (argc < 5) {
	//	printf("Usage: %s from to pageCapacity pageIndex\n", argv[0]);
	//	exit(-1);
	//}

	init();

	t = time(NULL);
	srand((unsigned int)t);
	for (i=0; i<5000; i++) {
		// start begin
		from = rand() % 3000 + 1;
		to = rand() % 5000 + from;
		pageCap = 5;
		pageIndex = 1;
		query(from, to, pageCap, pageIndex);
	}

	destroy();
	return 0;
}

