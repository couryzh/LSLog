/*******************************************************************************
* FileName : log.h
* Author : zhangpp
*******************************************************************************/
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define myLog(fmt, ...) \
    do {                        \
        printf("%s:(%d) " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define myLogErr(fmt, ...) \
    do {                        \
        fprintf(stderr, "%s:(%d) " fmt : %s"\n", __FILE__, __LINE__, ##__VA_ARGS__, strerror(errno)); \
    } while (0)

