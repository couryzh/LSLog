#include "LSLog.h"
#include "LSLogMemPool.h"
#include "LSLogFileImpl.h"
#include "LSLogFile.h"

#include <unistd.h>

bool test0();
bool testFile();
bool testFile2();



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
	testFile();

	destroy();
	return 0;
}

bool testFile2()
{
	int i;
	char event[32];


//#define LSLOG_MAX_LOG_NUM    		5
	for (i=1; i<=7; i++) {
		sprintf(event, "设置-密码-%d", i);
		log->log(OPE_LOG, i*2, (char *)"admin", event);
	}

	sleep(1);
	//log->logFile[0]->print();
	strcpy(event, "设置-密码-5");
	log->log(OPE_LOG, 5, (char *)"admin", event);
	sleep(1);
	//log->logFile[0]->print();


	for (i=8; i<=9; i++) {
		sprintf(event, "设置-密码-%d", i);
		log->log(OPE_LOG, i*2, (char *)"admin", event);
	}
	sleep(1);
	printf("in the end\n");
	log->logFile[0]->print();

	return true;
}

bool testFile()
{
	int i;
	char event[32];

	for (i=1; i<=6; i++) {
		sprintf(event, "设置-密码-%d", i);
		log->log(OPE_LOG, i*2, (char *)"admin", event);
	}

	sleep(1);
	log->logFile[0]->print();
	strcpy(event, "设置-密码-5");
	log->log(OPE_LOG, 5, (char *)"admin", event);
	sleep(1);
	log->logFile[0]->print();

	return true;
}

bool test0()
{
	int total;
	int t, ts, te;
	LSLogInfo *logInfo, *p;

	ts = t = 2;
	log->log(OPE_LOG, t, (char *)"admin", (char*)"设置-密码-1");

	t += 3;
	log->log(OPE_LOG, t, (char *)"admin", (char*)"设置-用户-2");
	te = t+1;

	//sleep(2);
	t += 2;
	log->log(OPE_LOG, t, (char*)"admin", (char*)"设置-密码");

	//sleep(3);
	t += 3;
	log->log(OPE_LOG, t, (char *)"admin", (char*)"设置-用户");

	sleep(1);
	log->logFile[0]->print();

	t += 1;
	log->log(OPE_LOG, te, (char*)"ls", (char*)"设置-密码");
#if 0
#endif

	sleep(2);
	log->logFile[0]->print();
	//log->logFile[1]->print();

	total = log->queryLog(OPE_LOG, ts, te, 2, 1, logInfo);
	printf("query %d-%d\n", ts, te);
	printf("total: %d\n", total);
	for (p=logInfo; p != NULL; p=p->next) {
		printf("%5d %s %s\n", (int)(p->t), p->user, p->event);
	}
	printf("\n");

	return true;
}
