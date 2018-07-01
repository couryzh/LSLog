#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include "LSLogTemplate.h"
#include "LSLogFile.h"
#include "log.h"

// 符号最大长度
#define LSLOG_TEMPLATE_MAX_SYM_LEN  4

// 符号定界符
#define LSLOG_TPL_SYM_START '{'
#define LSLOG_TPL_SYM_END 	'}'

#define LSLOG_TPL_CH_DELIMTER '-'

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
	
	for (p=data; *p!='\0'; p++) {
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

const char *LSLogTemplate::shrink(const char *ch)
{
	std::map<std::string, std::string>::iterator it;

	it = chToSym.find(ch);
	if (it == chToSym.end())
		return NULL;
	return it->second.c_str();
}

int LSLogTemplate::newSym(char *tplBuf, int size, const char *sym)
{
	int len;
	
	*tplBuf = LSLOG_TPL_SYM_START; 
	len = strlen(sym);
	if (len + 2 >= size) return 0; 
	strcpy(&tplBuf[1], sym);
	tplBuf[len+1] = LSLOG_TPL_SYM_END;

	return len+2;
}

bool LSLogTemplate::shrink(const LSLogInfo *logInfo, LogStorageItem *logStorageItem)
{
	const char *sym;
	char *p, *p2, event[LSLOG_MAX_EVENT_LEN];
	char eventTpl[LSLOG_MAX_EVENT_TPL_LEN];
	int len, symLen;

	logStorageItem->t = logInfo->t;
	logStorageItem->user= logInfo->user;

	len = 0;
	memset(eventTpl, 0, LSLOG_MAX_EVENT_TPL_LEN);
	memcpy(event, logInfo->event, LSLOG_MAX_EVENT_LEN); 
	for (p=event; *p != '0'; p=p2) {
		p2 = strchr(event, LSLOG_TPL_CH_DELIMTER);
		if (p2 == NULL) break;

		*p2++ = '\0';	
		if ((sym = shrink(p)) == NULL) {
			myLog("未知文字，无法转成模板");
			return false;
		}

		symLen = newSym(&eventTpl[len], LSLOG_MAX_EVENT_TPL_LEN-1-len, sym);
		if (symLen == 0) {
			myLog("模板空间不足");
			return false;
		}
		len += symLen;
	}

	if (*p != '\0') {
		if ((sym = shrink(p)) == NULL) {
			myLog("未知文字，无法转成模板");
			return false;
		}

		symLen = newSym(&eventTpl[len], LSLOG_MAX_EVENT_TPL_LEN-1-len, sym);
		if (symLen == 0) {
			myLog("模板空间不足");
			return false;
		}
		len += symLen;
	}
	memcpy(logStorageItem->eventTpl, eventTpl, LSLOG_MAX_EVENT_TPL_LEN);
	logStorageItem->t = logInfo->t;
	logStorageItem->user = logInfo->user;
	return true;
}

const char *LSLogTemplate::expand(const char *sym)
{
	std::map<std::string, std::string>::iterator it;

	it = symToCh.find(sym);
	if (it == symToCh.end())
		return NULL;
	return it->second.c_str();
}

bool LSLogTemplate::expand(const LogStorageItem *logStorageItem, LSLogInfo *&logInfo)
{
	bool inMatch; // 
	int iPos;
	char *sym;
	char *p, eventTpl[LSLOG_MAX_EVENT_TPL_LEN];
	const char *ch;

	iPos = 0;
	inMatch = false;

	memcpy(eventTpl, logStorageItem->eventTpl, LSLOG_MAX_EVENT_TPL_LEN);
	for (p = eventTpl; *p!= '\0'; p++) {
		if (!inMatch) {
			if (*p != LSLOG_TPL_SYM_START) {
				if (iPos < LSLOG_MAX_EVENT_LEN-1) 
					logInfo->event[iPos++] = *p;
			}
			else {
				inMatch = true;
				sym = p+1;
			}
		}
		else {
			if (*p == LSLOG_TPL_SYM_END) {
				*p = '\0';
				inMatch = false;
				ch = expand(sym);
				if (ch) {
					if (strlen(ch) + iPos < LSLOG_MAX_EVENT_LEN -1) {
						strcpy(&logInfo->event[iPos], ch);
						iPos += strlen(ch);
					}
					else  {
						myLog("expand character too long");
						break;
					}
				}
				else {
					myLog("%s cannot expand", sym);
				}
			}
		}
	}

	if (*p == '\0') {
		logInfo->t = logStorageItem->t;
		logInfo->user = logStorageItem->user;
		return true;
	}

	return false;
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
