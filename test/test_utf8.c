#include "../src/minitest.h"
#include "../src/utf8.h"

#include <windows.h>

TEST(test, utf8) {
    SetConsoleOutputCP(65001);

    const char *s   = "Hello 世界 🌍▲▼█🚀◼↩↵→☑☐⯆⯈";
    utf8_t          buf[256];
    int n = str_to_utf8(s, buf, 256);
    if (n < 0) {
        puts("invalid UTF-8");
        return ;
    }
    for (int i = 0; i < n; ++i) {
        char tmp[5] = {0};            
        for (int j = 0; j < buf[i].len; ++j)
            tmp[j] = buf[i].bytes[j];

        printf("char %d: len=%u width=%u  ", i, buf[i].len, buf[i].width);
        for (int j = 0; j < buf[i].len; ++j)
            printf("%02x ", (unsigned char)buf[i].bytes[j]);
        printf(" %s\n", tmp);      
    }
}
