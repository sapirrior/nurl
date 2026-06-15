#ifndef NURL_HEADERS_H
#define NURL_HEADERS_H

#include "nurl.h"
#include "engine/utils/nurl_error.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char **entries;   /* "Key: Value\r\n" strings */
    size_t count;
    size_t capacity;
} NurlHeaderList;

NurlHeaderList *nurl_headers_new(void);
nurl_err_t      nurl_headers_add(NurlHeaderList *h, const char *key, const char *value);
nurl_err_t      nurl_headers_add_raw(NurlHeaderList *h, const char *line);  /* "Key: Value" */
bool            nurl_headers_has(const NurlHeaderList *h, const char *key); /* case-insensitive */
nurl_err_t      nurl_headers_apply_auth(NurlHeaderList *h, const CommonArgs *a);
nurl_err_t      nurl_headers_apply_common(NurlHeaderList *h, const CommonArgs *a);
char           *nurl_headers_serialize(const NurlHeaderList *h);            /* heap alloc, caller frees */
void            nurl_headers_free(NurlHeaderList *h);

typedef struct {
    char   **keys;       /* canonical-case key strings, heap-owned */
    char   **values;     /* value strings, heap-owned */
    size_t   count;
    size_t   capacity;
} NurlHeaderMap;

NurlHeaderMap *nurl_headermap_new(void);
nurl_err_t     nurl_headermap_set(NurlHeaderMap *m, const char *key, const char *value);
nurl_err_t     nurl_headermap_append(NurlHeaderMap *m, const char *key, const char *value);
bool           nurl_headermap_has(const NurlHeaderMap *m, const char *key);
char          *nurl_headermap_serialize(const NurlHeaderMap *m);
void           nurl_headermap_free(NurlHeaderMap *m);

#endif /* NURL_HEADERS_H */
