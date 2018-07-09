#ifndef LSLOGCOMMON_H
#define LSLOGCOMMON_H

#include <ctime>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <list>
#include <fcntl.h>

/*
 * 日志模块
 */

// 最大日志条数
#define LSLOG_MAX_LOG_NUM    		1000

// 日志信息最大长度
#define LSLOG_MAX_EVENT_LEN  		64

// 用户名最大长度
#define LSLOG_MAX_USER_LEN			8

// 信息模板最大长度
#define LSLOG_MAX_EVENT_TPL_LEN  	18

// 用户名模板最大长度
#define LSLOG_MAX_USER_TPL_LEN  	2

// 文件路径最大长度
#define LSLOG_MAX_PATH_LEN	 		128

// 日志文件存储目录
#define LSLOG_WORK_PATH				"log/"

// 操作日志
#define LSLOG_OPE_FILE 		"ls_ope.log"

// 配置日志
#define LSLOG_CFG_FILE 		"ls_cfg.log"

// 运行日志
#define LSLOG_RUN_FILE 		"ls_run.log"

// 异常运行日志
#define LSLOG_ABNOR_FILE 	"ls_abnor.log"

// 模板信息文件
// 模板只加不删，否则原有信息将无法解读
#define LSLOG_CFG_TEMPLATE 	"ls.tpl"

enum LogType {
	OPE_LOG, 	// 操作日志
	CFG_LOG, 	// 配置日志
	RUN_LOG, 	// 运行日志
	ABNOR_LOG,	// 异常日志 
	LOG_NUM		// 日志类型个数，没有这种日志类型
};

struct LSLogInfo {
	LogType type;
	bool isTpl;
	time_t t;
	char event[LSLOG_MAX_EVENT_LEN+1];
	struct LSLogInfo *next;
};

#endif
