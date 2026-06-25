#ifndef NUT_POOL_H
#define NUT_POOL_H

#include "engine/nut_engine_request.h"
#include "nut_stream.h"
#include <time.h>
#include <stdbool.h>

#define NUT_POOL_MAX 8  /* max cached connections */

typedef struct {
    char         host[256];
    int          port;
    bool         is_tls;
    NutStream  *stream;
    bool         in_use;
    time_t       last_used;   /* for idle eviction */
} NutPoolEntry;

typedef struct NutConnPool {
    NutPoolEntry entries[NUT_POOL_MAX];
} NutConnPool;

NutConnPool  *nut_pool_create(void);
void           nut_pool_destroy(NutConnPool *pool);

/* Acquire a cached connection or open a new one.
 * Returns NUT_OK on success; *stream is populated. */
nut_err_t     nut_pool_acquire(NutConnPool *pool,
                   const char *host, int port, bool is_tls,
                   const NutRequest *req,
                   NutStream **stream);

/* Return a connection to the pool after a successful keep-alive request. */
void           nut_pool_release(NutConnPool *pool,
                   const char *host, int port,
                   NutStream *stream);

/* Permanently close and evict a connection (on error or Connection: close). */
void           nut_pool_evict(NutConnPool *pool, NutStream *stream);

#endif /* NUT_POOL_H */
