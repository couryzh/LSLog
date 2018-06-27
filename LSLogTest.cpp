#include "LSLog.h"
#include "LSLogMemPool.h"
#include "LSLogFileImpl.h"
#include "LSLogFile.h"

#include <unistd.h>

int main()
{
	int total;
	int t, ts, te;
	LSLogInfo *logInfo, *p;
	LSLogMemPool *pool = new LSLogMemPool(10);
	LSLogFileImpl *log = new LSLogFileImpl(pool);		
	
	ts = t = 2;
	log->log(OPE_LOG, t, 1, "设置-密码");
	log->log(CFG_LOG, t, 2, "设置-密码");

	sleep(3);
	t += 3;
	log->log(OPE_LOG, t, 1, "设置-用户");
	te = t+1;

	sleep(2);
	t += 2;
	log->log(OPE_LOG, t, 1, "设置-密码");

	sleep(3);
	t += 3;
	log->log(OPE_LOG, te, 1, "设置-用户");

	sleep(1);
	t += 1;
	log->log(OPE_LOG, t, 1, "设置-用户");

	sleep(2);
	log->logFile[0]->dump();
	log->logFile[1]->dump();

	total = log->queryLog(OPE_LOG, ts, te, 2, 1, logInfo);
	printf("query %d-%d\n", ts, te);
	printf("total: %d\n", total);
	for (p=logInfo; p != NULL; p=p->next) {
		printf("%5d %2d %s\n", (int)(p->t), p->user, p->event);
	}
	printf("\n");

	delete pool;
	delete log;
	
	return 0;
}
