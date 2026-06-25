#ifndef NUT_DISPATCH_H
#define NUT_DISPATCH_H

#include "nut.h"
#include "engine/nut_ctx.h"

int nut_dispatch(NutCtx *ctx, const char *method, const char *url, const CommonArgs *args);

#endif /* NUT_DISPATCH_H */
