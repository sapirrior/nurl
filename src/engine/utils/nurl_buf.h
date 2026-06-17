#ifndef NURL_BUF_H
#define NURL_BUF_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} NurlBuf;

void  nurl_buf_init(NurlBuf *b);
bool  nurl_buf_append(NurlBuf *b, const char *s, size_t n);
bool  nurl_buf_printf(NurlBuf *b, const char *fmt, ...);
void  nurl_buf_free(NurlBuf *b);
char *nurl_buf_take(NurlBuf *b);

#endif /* NURL_BUF_H */
