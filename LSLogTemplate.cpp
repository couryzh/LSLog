#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include "log.h"
#include "LSLogTemplate.h"

// 符号最大长度
#define LSLOG_TEMPLATE_MAX_SYM_LEN  4

// 符号定界符
#define LSLOG_TPL_SYM_START '{'
#define LSLOG_TPL_SYM_END 	'}'

#define LSLOG_TPL_CH_DELIMTER '-'

#define LSLOG_TPL_UNKOWN_USER "uu"

LSLogTemplate::LSLogTemplate()
{
	char tplPath[LSLOG_MAX_PATH_LEN+1];

	if (snprintf(tplPath, LSLOG_MAX_PATH_LEN+1, "%s%s", LSLOG_WORK_PATH, LSLOG_CFG_TEMPLATE) >= LSLOG_MAX_PATH_LEN+1) {
		myLog("too long template path: %s%s",  LSLOG_WORK_PATH, LSLOG_CFG_TEMPLATE); 
	}
	load(tplPath);
}

LSLogTemplate::~LSLogTemplate()
{
	clear();
}

bool LSLogTemplate::load(const char *tplFile)
{
	char line[64];
	char *sym, *ch;
	FILE *tplFp;

	strcpy(templatePath, tplFile);

	tplFp = fopen(tplFile, "rb");
	if (tplFp == NULL) {
		myLogErr("open template file %s", tplFile);
		return false;
	}

	while (fgets(line, 64, tplFp)) {
		if (!isLegalTpl(line)) continue;

		split(line, sym, ch);	
		if (sym == NULL || ch == NULL) continue;

		addToken(sym, ch);
	}
	
	fclose(tplFp);

	return true;
}

void LSLogTemplate::clear()
{
	symToCh.clear();
	std::vector<char *>::iterator it = symVec.begin();
	while (it != symVec.end()) {
		delete [] (*it);
		++it;
	}
}

void LSLogTemplate::reLoad(const char *tplFile)
{
	char tplPath[LSLOG_MAX_PATH_LEN+1];

	if (snprintf(tplPath, LSLOG_MAX_PATH_LEN+1, "%s%s", LSLOG_WORK_PATH, tplFile) >= LSLOG_MAX_PATH_LEN+1) {
		myLog("too long template path: %s%s",  LSLOG_WORK_PATH, tplFile);
		//exit(-1);
	}
	clear();
	load(tplPath);
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
		return false;
	}
	strcpy(mCh, ch);
	symToCh[mSym] = mCh;
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

bool LSLogTemplate::isLegalTpl(const char *data)
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

bool LSLogTemplate::isLegalEvent(const char *eventSym)
{
	char *p, *p1, *p2;
	char eventTpl[LSLOG_MAX_EVENT_TPL_LEN+1];
	strcpy(eventTpl, eventSym);

	/* 检查{n3}这种模板
	 * 1) 格式是否完整 例如没有'}'
	 * 2) n3是否存在对应文字
	 */
	for (p=eventTpl; *p!= '\0'; p=p2) {
		// 其他非模板文字，合法
		if ((p1=strchr(p, LSLOG_TPL_SYM_START)) == NULL)
			break;
		// 1)
		if ((p1[1]) == '0') return false;
		if ((p2 = strchr(p1+1, LSLOG_TPL_SYM_END)) == NULL)
			return false;
		*p2++ = '\0';

		// 2)
		if (expand(p1+1) == NULL) return false;
	}

	return true;
}

bool LSLogTemplate::shrink(const LSLogInfo *logInfo, LogStorageItem *logStorageItem)
{
	if (!logInfo->isTpl || !isLegalEvent(logInfo->event))
		return false;
	
	memcpy(logStorageItem->eventTpl, logInfo->event, LSLOG_MAX_EVENT_TPL_LEN+1);
	myLog("eventTpl: %s", logStorageItem->eventTpl);
	logStorageItem->t = logInfo->t;

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
	char *p, eventTpl[LSLOG_MAX_EVENT_TPL_LEN+1];
	const char *ch;

	iPos = 0;
	inMatch = false;

	memcpy(eventTpl, logStorageItem->eventTpl, LSLOG_MAX_EVENT_TPL_LEN+1);
	for (p = eventTpl; *p!= '\0'; p++) {
		if (!inMatch) {
			if (*p != LSLOG_TPL_SYM_START) {
				if (iPos < LSLOG_MAX_EVENT_LEN) 
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
					if (strlen(ch) + iPos < LSLOG_MAX_EVENT_LEN) {
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
		logInfo->event[iPos] = '\0';
		logInfo->isTpl = false;

		logInfo->t = logStorageItem->t;
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
}
