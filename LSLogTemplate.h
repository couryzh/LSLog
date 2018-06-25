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
	LSLogTemplate(const char *tplFile);
	~LSLogTemplate();

	void reLoad(const char *tplFile);

	const char *shrink(char *ch);
	void shrink(const LSLogInfo *logInfo, LogStorageItem *logStorageItem);

	const char *expand(char *sym);
	void expand(const LogStorageItem *logStorageItem, LSLogInfo *logInfo);

private:
	void load(const char *tplFile);
	void clear();
	bool isLegal(const char *data);
	void split(char *data, char *&sym, char *&ch);
	bool addToken(char *sym, char *ch);

private:
	char templateFile[TEMPLATE_FILE_SIZE];
	std::map<std::string, std::string> symToCh;
	std::map<std::string, std::string> chToSym;
	std::vector<char *> symVec;
	std::vector<char *> chVec;
};
#endif
