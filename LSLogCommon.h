#ifndef LSLOGCOMMON_H
#define LSLOGCOMMON_H

#include <ctime>
#include <cstdio>
#include <list>

#define LSLOG_MAX_EVENT_LEN  64
#define LSLOG_MAX_LOG_NUM    20000
#define LSLOG_WORK_PATH	   "log/"

struct LSLogInfo {
	time_t t;
	char user;
	char event[LSLOG_MAX_EVENT_LEN];
};

#endif
