#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdarg.h>

static inline void log_print(const char *fmt, ...)
{
    FILE *fp = fopen("log.txt", "a");
    if (!fp) return;

    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);

    fputc('\n', fp);
    fclose(fp);
}

#define LOG(fmt, ...)  log_print(fmt, ##__VA_ARGS__)

#endif // __LOG_H__