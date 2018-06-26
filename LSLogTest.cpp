#include "LSLog.h"
#include "LSLogMemPool.h"
#include "LSLogFileImpl.h"

#include <unistd.h>

int main()
{
	time_t t;
	LSLogMemPool *pool = new LSLogMemPool(10);
	LSLog *log = new LSLogFileImpl(pool);		
	
	t = time(NULL);
	log->log(OPE_LOG, t, 1, "设置密码");
	log->log(CFG_LOG, t, 1, "设置密码");

	sleep(5);

	delete pool;
	delete log;
	
	return 0;
}
