#include <assert.h>
#include <stdio.h>
#include "LSLogTemplate.h"

LSLogTemplate *tpl;

void isLegalEventTest()
{
	assert(tpl->isLegalEvent("{s}{up}"));
	assert(tpl->isLegalEvent("{s}a{up}bb"));
	assert(tpl->isLegalEvent("{s}{up}安全"));
	assert(tpl->isLegalEvent("{s}{n5}") == false);
	assert(tpl->isLegalEvent("{s}{") == false);
	assert(tpl->isLegalEvent("{s{up") == false);
	assert(tpl->isLegalEvent("{}p{up}") == false);

	printf("isLegalEvent pass !\n");
}

int main()
{
	tpl = new LSLogTemplate();

	isLegalEventTest();

	tpl->reLoad("ls2.tpl");
	isLegalEventTest();

	return 0;
}

/* g++ -Wall -g LSLogTplTest.cpp LSLogTemplate.cpp -I. -o LSLogTplTest */
