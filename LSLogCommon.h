#ifndef LSLOGCOMMON_H
#define LSLOGCOMMON_H

#include <ctime>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <list>
#include <fcntl.h>

#define LSLOG_MAX_EVENT_LEN  64
#define LSLOG_MAX_LOG_NUM    20000
#define LSLOG_WORK_PATH	   "log/"
#define LSLOG_CFG_TEMPLATE "event.tpl"

struct LSLogInfo {
	time_t t;
	char user;
	char event[LSLOG_MAX_EVENT_LEN];
	struct LSLogInfo *next;
};

struct LogStorageItem {
	time_t t;
	char user;
	char eventTpl[11];
};

#endif
