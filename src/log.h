#ifndef __LOG_H__
#define __LOG_H__

#define LOG(fmt, ...) do { \
    static int first = 1; \
    FILE *fp = fopen("debug.log", first ? "w" : "a"); \
    if (fp) { \
        fprintf(fp, "%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        fclose(fp); \
        first = 0; \
    } \
} while(0)

#endif /* __LOG_H__ */