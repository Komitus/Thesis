#ifndef __STRUCTS__
#define __STRUCTS__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include "vector.h"

#define FAIL_STATUS -1
#define SUCCES_STATUS 0

#define LOGGING

#ifdef LOGGING
#define DEBUG_LOG(fmt, ...)                \
    do                                     \
    {                                      \
        fprintf(stderr, fmt, __VA_ARGS__); \
    } while (0)
#else
#define DEBUG_LOG(fmt, ...)
#endif

#define NO_OPTION 0x0
#define FROM_FILE 0x01
#define TO_FILE 0x02
#define QUIET_MODE 0x04
#define FOREACH_ALGO(ALGO) \
    ALGO(APPROX = 0x08)    \
    ALGO(MIP = 0x10)

#define GENERATE_ENUM(ENUM) ENUM,

enum
{
    FOREACH_ALGO(GENERATE_ENUM)
};

typedef struct _ObjWithQuantity
{
    size_t length;
    size_t quantity;
} ObjWithQuantity;

typedef struct _StockConfig
{
    size_t spaceLeft;
    size_t config[];
} StockConfig;

typedef struct _ProblemInstance
{
    ObjWithQuantity *arrOfObjs;
    void *ptrToSolutionSpecified;
    size_t numOfConfigs;
    size_t numOfTypes;
    size_t stockLength;
    size_t numOfAllObjs;
    size_t minNumbOfStocks;
} ProblemInstance;

typedef struct _IO_Info
{
    char *inFilename;
    char *outFilename;
    uint8_t options;
    FILE *inFile;
    FILE *outFile;
} IO_Info;

#endif