#ifndef __CORE_LOG_H__
#define __CORE_LOG_H__

#include <stdio.h>

#define TT_PRINT_RED       "\033[31m"
#define TT_PRINT_YELLOW    "\033[33m"
#define TT_PRINT_GREEN     "\033[32m"
#define TT_PRINT_BLUE      "\033[34m"
#define TT_PRINT_WHITE     "\033[37m"
#define TT_PRINT_GRAY      "\033[90m"
#define TT_PRINT_CYAN      "\033[36m"
#define TT_PRINT_PURPLE    "\033[35m"
#define TT_PRINT_NONE      "\033[0m"

#define LOG(...)           printf(__VA_ARGS__); printf(TT_PRINT_NONE "\n")

#define LOG_INFO(...)      printf(TT_PRINT_GREEN "[ INFO] "); printf(__VA_ARGS__); printf(TT_PRINT_NONE "\n")
#define LOG_DEBUG(...)     printf(TT_PRINT_GRAY "[DEBUG] "); printf(__VA_ARGS__); printf(TT_PRINT_NONE "\n")
#define LOG_WARN(...)      printf(TT_PRINT_YELLOW "[ WARN] "); printf(__VA_ARGS__); printf(TT_PRINT_NONE "\n")
#define LOG_ERROR(...)     printf(TT_PRINT_RED "[ERROR] "); printf(__VA_ARGS__); printf(TT_PRINT_NONE "\n")
#define LOG_TEST(...)     printf(TT_PRINT_CYAN "[ TEST] "); printf(__VA_ARGS__); printf(TT_PRINT_NONE "\n")


#define LOG_FILE_PATH      "../out/matrix_c.log"
#define LOG_FILE_MAX_SIZE  1024 * 1024 * 10

void log_file(const char* format, ...);
void set_log_file(const char* path, int max_size);
void close_log_file();

#define SET_LOG_FILE(...)   set_log_file(__VA_ARGS__)
#define LOG_FILE(...)       log_file(__VA_ARGS__)
#define CLOSE_LOG_FILE      close_log_file()

#endif // __CORE_LOG_H__