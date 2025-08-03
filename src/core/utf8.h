#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>
#include <stdint.h>

uint32_t utf8_decode(const char **s);
int      utf8_encode(uint32_t cp, char dst[4]);
size_t   utf8_len(const char *s);
int      utf8_width(uint32_t cp);
int      utf8_swidth(const char *s);
int      utf8_validate(const char *s, size_t n);

#endif /* UTF8_H */