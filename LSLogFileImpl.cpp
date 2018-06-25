#include "LSLogFileImpl.h"

LSLogFileImpl::LSLogFileImpl()
{
	logFile = new LSLogFile();
	cacheQueue = new LSLogCacheQueue();
}

LSLogFileImpl::~LSLogFileImpl()
{
	delete logFile;
	delete cacheQueue;
}

bool LSLogFileImpl:: log(time_t t, char user, const char *event)
{
	return false;
}

int LSLogFileImpl::queryLog(time_t from, time_t to, int pageCapacity, int pageIndex, struct LSLogInfo *&logInfos)
{
	return 0;
}

void LSLogFileImpl::saveLog()
{
	
}
