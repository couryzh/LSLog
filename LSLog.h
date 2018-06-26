#ifndef LSLOG_H
#define LSLOG_H

#include "LSLogCommon.h"

class LSLog {
public:
	virtual ~LSLog();

	virtual bool log(LogType type, time_t t, char user, const char *event) = 0;

	virtual int queryLog(time_t from, time_t to, int pageCapacity, int pageIndex, struct LSLogInfo *&logInfos) = 0; 

};

#endif
