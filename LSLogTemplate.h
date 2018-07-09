#ifndef LSLOGTEMPLATE_H
#define LSLOGTEMPLATE_H

#include <map>
#include <vector>
#include <string>
#include "LSLogCommon.h"
#include "LSLogFile.h"

#define TEMPLATE_FILE_SIZE  128

//class LSLogFile;

class LSLogTemplate {
public:
	LSLogTemplate();
	~LSLogTemplate();

	// 重新加载定义的模板文件
	void reLoad(const char *tplFile);

	// 已转成模板的信息是否合法(是否有不识别的模板)
	bool isLegalEvent(const char *eventSym);

	// 压缩成模板
	bool shrink(const LSLogInfo *logInfo, LogStorageItem *logStorageItem);
	// 由模板还原
	bool expand(const LogStorageItem *logStorageItem, LSLogInfo *&logInfo);

private:
	// 由.tpl文件加载已定义的模板
	bool load(const char *tplFile);

	// 将这对信息添加到map
	bool addToken(char *sym, char *ch);

	// tpl文件某行是否合法
	bool isLegalTpl(const char *data);
	void split(char *data, char *&sym, char *&ch);
	void clear();

	// 符号转文字
	const char *expand(const char *sym);

	void dump();

private:
	char templatePath[LSLOG_MAX_PATH_LEN+1];
	std::map<std::string, std::string> symToCh;
	std::vector<char *> symVec;
};
#endif
