#include <assert.h>
#include <stdio.h>
#include "LSLogTemplate.h"

LSLogTemplate *tpl;

void isLegalEventTest()
{
	assert(tpl->isLegalEvent("{p1}{n5}"));
	assert(tpl->isLegalEvent("{p1}a{n5}bb"));
	assert(tpl->isLegalEvent("{p1}{n5}安全"));
	assert(tpl->isLegalEvent("{p2}{n5}") == false);
	assert(tpl->isLegalEvent("{p}{") == false);
	assert(tpl->isLegalEvent("{p{n5") == false);
	assert(tpl->isLegalEvent("{}p{n5}") == false);

	printf("isLegalEvent pass !\n");
}

int main()
{
	tpl = new LSLogTemplate();

	isLegalEventTest();

	return 0;
}

/* g++ -Wall -g LSLogTplTest.cpp LSLogTemplate.cpp -I. -o LSLogTplTest */
