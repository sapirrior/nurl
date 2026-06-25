#ifndef NUT_BUF_H
#define NUT_BUF_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} NutBuf;

void  nut_buf_init(NutBuf *b);
bool  nut_buf_append(NutBuf *b, const char *s, size_t n);
bool  nut_buf_printf(NutBuf *b, const char *fmt, ...);
void  nut_buf_free(NutBuf *b);
char *nut_buf_take(NutBuf *b);

#endif /* NUT_BUF_H */
