#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include "LSLogTemplate.h"
#include "LSLogFile.h"
#include "log.h"

// 符号最大长度
#define LSLOG_TEMPLATE_MAX_SYM_LEN  4

LSLogTemplate::LSLogTemplate()
{
	char tplPath[LSLOG_MAX_PATH_LEN];

	if (snprintf(tplPath, LSLOG_MAX_PATH_LEN, "%s%s", LSLOG_WORK_PATH, LSLOG_CFG_TEMPLATE) >= LSLOG_MAX_PATH_LEN) {
		myLog("too long template path: %s%s",  LSLOG_WORK_PATH, LSLOG_CFG_TEMPLATE); 
		exit(-1);
	}
	load(tplPath);
}

void LSLogTemplate::load(const char *tplFile)
{
	char line[64];
	char *sym, *ch;
	FILE *tplFp;

	strcpy(templatePath, tplFile);

	tplFp = fopen(tplFile, "rb");
	if (tplFp == NULL) {
		myLogErr("open template file %s", tplFile);
		exit(-1);
	}

	while (fgets(line, 64, tplFp)) {
		if (!isLegal(line)) continue;		

		split(line, sym, ch);	
		if (sym == NULL || ch == NULL) continue;

		addToken(sym, ch);
	}
	
	fclose(tplFp);

	//dump();
}

bool LSLogTemplate::addToken(char *sym, char *ch)
{
	char *mSym, *mCh;
	int len;

	len = strlen(sym);
	mSym = new char[len+1];
	if (!mSym)
		return false;
	strcpy(mSym, sym);
	symVec.push_back(mSym);

	len = strlen(ch);
	mCh = new char[len+1];
	if (!mCh) {
		delete [] mSym;
		symVec.pop_back();
		return false;
	}
	strcpy(mCh, ch);
	chVec.push_back(mCh);

	symToCh[mSym] = mCh;
	chToSym[mCh] = mSym;
	return true;
}

void LSLogTemplate::split(char *data, char *&sym, char *&ch)
{
	char *p;
	int len;

	sym = ch = NULL;
	p = strchr(data, '=');
	if (p) {
		*p++ = '\0';
		sym = data;
		ch = p;
		len = strlen(ch);
		if (ch[len-1] == '\n')
			ch[len-1] = '\0';
		if (ch[len-2] == '\r')
			ch[len-2] = '\0';

		// 过长的符号定义，可能是编写错误，忽略之
		if (strlen(sym) >= LSLOG_TEMPLATE_MAX_SYM_LEN) 
			sym = NULL;
	}
}

bool LSLogTemplate::isLegal(const char *data)
{
	const char *p;
	
	if (!data) return false;
	
	for (p=data; p != '\0'; p++) {
		if (isspace(*p) == 0) break;
	}
	// 空白行
	if (*p == '\0') return false;

	// #开始的注释行
	if (*p == '#') return false;

	// 非 key=value表示
	if (strchr(p, '=') == NULL) return false;

	return true;
}

LSLogTemplate::~LSLogTemplate()
{
	clear();
}

void LSLogTemplate::clear()
{
	symToCh.clear();
	chToSym.clear();

	std::vector<char *>::iterator it = symVec.begin();
	while (it != symVec.end()) {
		delete [] (*it);
		++it;
	}

	it = chVec.begin();
	while (it != chVec.end()) {
		delete [] (*it);
		++it;
	}
}

void LSLogTemplate::reLoad(const char *tplFile)
{
	char tplPath[LSLOG_MAX_PATH_LEN];

	if (snprintf(tplPath, LSLOG_MAX_PATH_LEN, "%s%s", LSLOG_WORK_PATH, tplFile) >= LSLOG_MAX_PATH_LEN) {
		myLog("too long template path: %s%s",  LSLOG_WORK_PATH, tplFile); 
		exit(-1);
	}
	clear();
	load(tplPath);
}

const char *LSLogTemplate::shrink(char *ch)
{
	std::map<std::string, std::string>::iterator it;

	it = chToSym.find(ch);
	if (it == chToSym.end())
		return NULL;
	return it->second.c_str();
}

void LSLogTemplate::shrink(const LSLogInfo *logInfo, LogStorageItem *logStorageItem)
{
	
}

const char *LSLogTemplate::expand(char *sym)
{
	std::map<std::string, std::string>::iterator it;

	it = symToCh.find(sym);
	if (it == symToCh.end())
		return NULL;
	return it->second.c_str();
}

void LSLogTemplate::expand(const LogStorageItem *logStorageItem, LSLogInfo *&logInfo)
{

}

void LSLogTemplate::dump()
{
	std::map<std::string, std::string>::iterator it;

	printf("symbol to Charater\n");
	for (it=symToCh.begin(); it!=symToCh.end(); ++it) {
		printf("  %s: %s\n", it->first.c_str(), it->second.c_str());
	}

	printf("Charater to symbol\n");
	for (it=chToSym.begin(); it!=chToSym.end(); ++it) {
		printf("  %s: %s\n", it->first.c_str(), it->second.c_str());
	}
}
