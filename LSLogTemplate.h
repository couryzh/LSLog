#ifndef LSLOGTEMPLATE_H
#define LSLOGTEMPLATE_H

#include <map>
#include <vector>

#define TEMPLATE_FILE_SIZE  128

class LSLogTemplate {
public:
	LSLogTemplate(const char *tplFile);
	~LSLogTemplate();

	void reLoad(const char *tplFile);
	char *expand(const char *sym);
	char *shrink(const char *ch);

private:
	char templateFile[TEMPLATE_FILE_SIZE];
	std::map<char *, char *> symToCh;
	std::map<char *, char *> chToSym;
	std::vector<char *> symVec;
	std::vector<char *> chVec;
};
#endif
