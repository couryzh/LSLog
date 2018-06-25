#ifndef LSLOGFILE_H 
#define LSLOGFILE_H 

#include "LSLogCommon.h"
#include "LSLogTemplate.h"

class LSLogFile {
public:
	LSLogFile();
	bool save(time_t t, char user, char *event);

	int query(time_t from, time_t to, int blockSize, int blockIndex, char *&startAddr);

private:
	struct logFileHeader {
		short itemSize;
		int currentIndex;
	};

#define ITEM_START sizeof(struct logFileHeader)
	FILE *logFile;
	void *mapAddr;

	LSLogTemplate *logtpl;	
};

#endif
