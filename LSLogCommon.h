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
#define LSLOG_MAX_PATH_LEN	 128
#define LSLOG_MAX_LOG_NUM    20000
#define LSLOG_WORK_PATH		"log/"

#define LSLOG_OPE_FILE 		"ls_ope.log"
#define LSLOG_CFG_FILE 		"ls_cfg.log"
#define LSLOG_RUN_FILE 		"ls_run.log"
#define LSLOG_ABNOR_FILE 	"ls_abnor.log"
#define LSLOG_CFG_TEMPLATE 	"event.tpl"


enum LogType {
	OPE_LOG, 	// 操作日志
	CFG_LOG, 	// 配置日志
	RUN_LOG, 	// 运行日志
	ABNOR_LOG,	// 异常日志 
	LOG_NUM		// 日志类型个数，没有这种日志类型
};

struct LSLogInfo {
	LogType type;
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
