#ifndef LSLOGFILEIMPL_H
#define LSLOGFILEIMPL_H

#include "LSLog.h"
#include "LSLogFile.h"
#include "LSLogMemPool.h"
#include "LSLogCacheQueue.h"

class LSLogFileImpl : public LSLog {
public:
	enum {defaultMemPoolSize = 32};
	LSLogFileImpl();
	~LSLogFileImpl();

	bool log(time_t t, char user, const char *event);
	int queryLog(time_t from, time_t to, int pageCapacity,
			int pageIndex, struct LSLogInfo *&logInfos);

private:
	void saveLog();
	LSLogCacheQueue *cacheQueue;
	LSLogFile *logFile;
};

#endif
