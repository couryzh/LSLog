CC = g++
CXXFLAGS := -Wall -I.
COMPILE := -c
LINK := -lpthread

OBJ_PATH=obj
all: LSLogTest

LSLogTest: $(OBJ_PATH)/LSLogCacheQueue.o $(OBJ_PATH)/LSLog.o $(OBJ_PATH)/LSLogFile.o $(OBJ_PATH)/LSLogFileImpl.o \
	$(OBJ_PATH)/LSLogMemPool.o $(OBJ_PATH)/LSLogTemplate.o $(OBJ_PATH)/LSLogTest.o
	$(CC) $^ -o $@ $(LINK)

$(OBJ_PATH)/%.o: %.cpp
	$(CC) $(CXXFLAGS) $(COMPILE) $< -o $@ 

clean:
	rm -f *.o ~*
