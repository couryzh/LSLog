#include "LSLogMemPool.h"
#include "LSLogFileImpl.h"
#include "LSLogFile.h"

#include <unistd.h>

// ---------------------------------------
LSLogMemPool *pool;
LSLogFileImpl *log;


void init()
{
	pool = new LSLogMemPool(1000);
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
	time_t t;
	struct tm tm;

	t = time(NULL);
	localtime_r(&t, &tm);
	tm.tm_hour = 8; tm.tm_min = 0; tm.tm_sec = 0;
	t = mktime(&tm);
	
	for (i=0; i<=2000; i++) {
		sprintf(event, "{s}{up}%d", i);
		log->log(OPE_LOG, t, event);
		t += 2;
	}

	sleep(10);

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
