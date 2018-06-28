#ifndef LSLOGTEMPLATE_H
#define LSLOGTEMPLATE_H

#include <map>
#include <vector>
#include <string>
#include "LSLogCommon.h"

#define TEMPLATE_FILE_SIZE  128

class LSLogFile;

class LSLogTemplate {
public:
	LSLogTemplate();
	~LSLogTemplate();

	void reLoad(const char *tplFile);

	const char *shrink(const char *ch);
	bool shrink(const LSLogInfo *logInfo, LogStorageItem *logStorageItem);
	const char *expand(const char *sym);
	bool expand(const LogStorageItem *logStorageItem, LSLogInfo *&logInfo);

private:
	void load(const char *tplFile);
	void clear();
	bool isLegal(const char *data);
	void split(char *data, char *&sym, char *&ch);
	bool addToken(char *sym, char *ch);
	bool fillTpl(char eventTpl[], const unsigned size, int &len, const char *ch);
	int newSym(char *tplBuf, int size, const char *sym);

	void dump();

private:
	char templatePath[LSLOG_MAX_PATH_LEN+1];
	std::map<std::string, std::string> symToCh;
	std::map<std::string, std::string> chToSym;
	std::vector<char *> symVec;
	std::vector<char *> chVec;
};
#endif
