#ifndef NUT_CTX_H
#define NUT_CTX_H

#include "nut_pool.h"

typedef struct {
    NutConnPool *pool;
} NutCtx;

NutCtx *nut_ctx_create(void);
void     nut_ctx_destroy(NutCtx *ctx);

#endif /* NUT_CTX_H */
