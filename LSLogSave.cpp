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

	for (i=1; i<=5; i++) {
		sprintf(event, "设置-密码-%d", i);
		log->log(OPE_LOG, i*2, event);
	}

	sleep(3);
	log->log(OPE_LOG, 5, (char*)"设置-密码-5");
	sleep(1);

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
