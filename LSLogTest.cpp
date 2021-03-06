#include "LSLogMemPool.h"
#include "LSLogFileImpl.h"
//#include "LSLogFile.h"

#include <unistd.h>

bool test0();
bool testFile();
bool testFile2();
bool testFilex();


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

int main()
{
	init();

	// start begin
	testFilex();

	destroy();
	return 0;
}


bool testFilex()
{
	int i, total;
	char event[32];
	int ts, te;
	LSLogInfo *logInfo, *p;

	for (i=1; i<=10000; i++) {
		sprintf(event, "设置-密码-%d", i);
		log->log(OPE_LOG, i*2, event);
	}

	sleep(3);

	ts=80; te=100;
	total = log->queryLog(OPE_LOG, ts, te, 5, 1, logInfo);
	printf("query %d-%d\n", ts, te);
	printf("total: %d\n", total);
	for (p=logInfo; p != NULL; p=p->next) {
		printf("%5d %s\n", (int)(p->t), p->event);
	}
	printf("\n");

	return true;
}

bool testFile2()
{
	int i;
	char event[32];


//#define LSLOG_MAX_LOG_NUM    		5
	for (i=1; i<=7; i++) {
		sprintf(event, "设置-密码-%d", i);
		log->log(OPE_LOG, i*2, event);
	}

	sleep(1);
	//log->logFile[0]->printHeader();
	strcpy(event, "设置-密码-5");
	log->log(OPE_LOG, 5, event);
	sleep(1);
	//log->logFile[0]->printHeader();


	for (i=8; i<=9; i++) {
		sprintf(event, "设置-密码-%d", i);
		log->log(OPE_LOG, i*2, event);
	}
	sleep(1);
	printf("in the end\n");
	//log->logFile[0]->printHeader();

	return true;
}

bool testFile()
{
	int i, total;
	char event[32];
	int ts, te;
	LSLogInfo *logInfo, *p;

	for (i=1; i<=6; i++) {
		sprintf(event, "设置-密码-%d", i);
		log->log(OPE_LOG, i*2, event);
	}
	memset(event, 0, 32);
	sleep(1);

	/*
	log->logFile[0]->printHeader();
	strcpy(event, "设置-密码-5");
	log->log(OPE_LOG, 5, event);
	sleep(1); */

	//log->logFile[0]->printHeader();

	ts =2; te=6;
	total = log->queryLog(OPE_LOG, ts, te, 2, 1, logInfo);
	printf("query %d-%d\n", ts, te);
	printf("total: %d\n", total);
	for (p=logInfo; p != NULL; p=p->next) {
		printf("%5d %s\n", (int)(p->t), p->event);
	}
	printf("\n");

	return true;
}

bool test0()
{
	int total;
	int t, ts, te;
	LSLogInfo *logInfo, *p;

	ts = t = 2;
	log->log(OPE_LOG, t, (char*)"设置-密码-1");

	t += 3;
	log->log(OPE_LOG, t, (char*)"设置-用户-2");
	te = t+1;

	//sleep(2);
	t += 2;
	log->log(OPE_LOG, t, (char*)"设置-密码");

	//sleep(3);
	t += 3;
	log->log(OPE_LOG, t, (char*)"设置-用户");

	sleep(1);
	//log->logFile[0]->printHeader();

	t += 1;
	log->log(OPE_LOG, te, (char*)"设置-密码");
#if 0
#endif

	sleep(2);
	//log->logFile[0]->printHeader();
	//log->logFile[1]->printHeader();

	total = log->queryLog(OPE_LOG, ts, te, 2, 1, logInfo);
	printf("query %d-%d\n", ts, te);
	printf("total: %d\n", total);
	for (p=logInfo; p != NULL; p=p->next) {
		printf("%5d %s\n", (int)(p->t), p->event);
	}
	printf("\n");

	return true;
}
