#include "LSLog.h"
#include "LSLogFileImpl.h"

int main()
{
	time_t t;
	LSLog *log = new LSLogFileImpl();		

	t = time(NULL);
	log->log(t, 1, "修改密码");


	delete log;

	return 0;
}
