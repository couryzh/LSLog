CC = g++
CXXFLAGS := -Wall -g -I.
COMPILE := -c
LINK := -lpthread
NOPTIMIZE = -O0
OPTIMIZE = -O2

OBJ_PATH=obj
all: LSLogTest LSLogSave LSLogQuery

LSLogTest: $(OBJ_PATH)/LSLogCacheQueue.o $(OBJ_PATH)/LSLog.o $(OBJ_PATH)/LSLogFile.o $(OBJ_PATH)/LSLogFileImpl.o \
	$(OBJ_PATH)/LSLogMemPool.o $(OBJ_PATH)/LSLogTemplate.o $(OBJ_PATH)/LSLogTest.o
	$(CC) $^ -o $@ $(LINK)

LSLogSave: $(OBJ_PATH)/LSLogCacheQueue.o $(OBJ_PATH)/LSLog.o $(OBJ_PATH)/LSLogFile.o $(OBJ_PATH)/LSLogFileImpl.o \
	$(OBJ_PATH)/LSLogMemPool.o $(OBJ_PATH)/LSLogTemplate.o $(OBJ_PATH)/LSLogSave.o
	$(CC) $^ -o $@ $(LINK)

LSLogQuery: $(OBJ_PATH)/LSLogCacheQueue.o $(OBJ_PATH)/LSLog.o $(OBJ_PATH)/LSLogFile.o $(OBJ_PATH)/LSLogFileImpl.o \
	$(OBJ_PATH)/LSLogMemPool.o $(OBJ_PATH)/LSLogTemplate.o $(OBJ_PATH)/LSLogQuery.o
	$(CC) $^ -o $@ $(LINK)

$(OBJ_PATH)/%.o: %.cpp LSLogCommon.h
	$(CC) $(CXXFLAGS) $(NOPTIMIZE) $(COMPILE) $< -o $@ 

clean:
	rm -f $(OBJ_PATH)/*.o ~* LSLogTest LSLogSave LSLogQuery
