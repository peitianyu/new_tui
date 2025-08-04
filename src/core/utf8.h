#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>
#include <stdint.h>
#include "log.h"

uint32_t utf8_decode(const char **s);
int      utf8_encode(uint32_t cp, char dst[4]);
size_t   utf8_len(const char *s);
int      utf8_swidth_len(const char *s, size_t byte_len);
int      utf8_width(uint32_t cp);
int      utf8_swidth(const char *s);
int      utf8_valid(const char *s, size_t n);
size_t   utf8_chr_len(const char *s);
size_t   utf8_prev(const char *s, size_t cursor);
size_t   utf8_advance(const char *s, size_t cursor, int n);
size_t   utf8_trunc_width(const char *s, int max_w);

#endif /* UTF8_H */