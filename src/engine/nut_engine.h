#ifndef NUT_ENGINE_H
#define NUT_ENGINE_H

#include "engine/nut_engine_request.h"
#include "nut_http.h"

#include "nut_ctx.h"

typedef struct {
    double connect_time_sec;
    int    num_redirects;
} NutOperationStats;

int nut_engine_execute_request(
    NutCtx *ctx,
    NutRequest *req,
    nut_http_response_t **out_response,
    char **out_effective_url,
    NutOperationStats *out_stats
);

#endif /* NUT_ENGINE_H */
