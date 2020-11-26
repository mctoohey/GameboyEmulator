#ifndef SRC_LOGGING_H_
#define SRC_LOGGING_H_
#include <stdio.h>

#define LOGGING_LEVEL 0

#if LOGGING_LEVEL >= 1
    #define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_ERROR(args, ...)
#endif

#if LOGGING_LEVEL >= 2
    #define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_INFO(args, ...)
#endif

#if LOGGING_LEVEL >= 3
    #define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_DEBUG(args, ...)
#endif


#endif  // SRC_LOGGING_H_
