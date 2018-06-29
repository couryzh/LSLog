#include "LSLog.h"
#include "LSLogMemPool.h"
#include "LSLogFileImpl.h"
#include "LSLogFile.h"

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

bool save()
{
	int i;
	char event[32];

	for (i=1; i<=20; i++) {
		sprintf(event, "设置-密码-%d", i);
		log->log(OPE_LOG, i*2, (char *)"admin", event);
	}

	sleep(3);
	log->log(OPE_LOG, 25, (char *)"admin", (char *)"设置-密码-25");
	sleep(1);
	log->logFile[0]->printFile();

	return true;
}

int main()
{
	init();

	// start begin
	save();

	destroy();
	return 0;
}
