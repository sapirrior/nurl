#include "nut_ctx.h"
#include <stdlib.h>

NutCtx *nut_ctx_create(void) {
    NutCtx *ctx = calloc(1, sizeof(NutCtx));
    if (!ctx) return NULL;
    
    ctx->pool = nut_pool_create();
    if (!ctx->pool) {
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

void nut_ctx_destroy(NutCtx *ctx) {
    if (!ctx) return;
    if (ctx->pool) {
        nut_pool_destroy(ctx->pool);
    }
    free(ctx);
}
