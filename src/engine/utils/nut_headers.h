#ifndef NUT_HEADERS_H
#define NUT_HEADERS_H

#include "errors/nut_error.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char   **keys;       /* canonical-case key strings, heap-owned */
    char   **values;     /* value strings, heap-owned */
    size_t   count;
    size_t   capacity;
} NutHeaderMap;

NutHeaderMap *nut_headermap_new(void);
nut_err_t     nut_headermap_set(NutHeaderMap *m, const char *key, const char *value);
nut_err_t     nut_headermap_append(NutHeaderMap *m, const char *key, const char *value);
nut_err_t     nut_headermap_add_raw(NutHeaderMap *m, const char *line);
bool           nut_headermap_has(const NutHeaderMap *m, const char *key);
char          *nut_headermap_serialize(const NutHeaderMap *m);
void           nut_headermap_free(NutHeaderMap *m);

#endif /* NUT_HEADERS_H */
