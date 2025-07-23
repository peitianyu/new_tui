#include "c_log.h"
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>


static FILE* g_log_file = NULL;
static int g_log_file_size = 0;

static const char* g_log_file_path = LOG_FILE_PATH;
static int g_log_file_max_size = LOG_FILE_MAX_SIZE;

void set_log_file(const char* path, int max_size)
{
    g_log_file_path = path;
    g_log_file_max_size = max_size;
}

void log_file(const char* format, ...)
{
    if(!g_log_file) {
        g_log_file = fopen(g_log_file_path, "a+");
        if(!g_log_file) return ;
    }
    if(g_log_file_size > g_log_file_max_size) {
        fclose(g_log_file);
        g_log_file = fopen(g_log_file_path, "w+");
        if(!g_log_file) return ;
        g_log_file_size = 0;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);
    fprintf(g_log_file, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, 
        tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000));
    va_list args;
    va_start(args, format);
    vfprintf(g_log_file, format, args);
    va_end(args);
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
    g_log_file_size++;
}

void close_log_file()
{
    if(g_log_file) fclose(g_log_file);
    g_log_file = NULL;
    g_log_file_size = 0;
}