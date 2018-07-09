#include "LSLogMemPool.h"
#include "LSLogFileImpl.h"
//#include "LSLogFile.h"

#include <string.h>
#include <unistd.h>

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
	int from, to;
	int pageCap, pageIndex;

	if (argc < 5) {
		printf("Usage: %s from to pageCapacity pageIndex\n", argv[0]);
		exit(-1);
	}
	from = atoi(argv[1]);
	to = atoi(argv[2]);
	pageCap = atoi(argv[3]);
	pageIndex = atoi(argv[4]);

	init();


	// start begin
	query(from, to, pageCap, pageIndex);

	destroy();
	return 0;
}

