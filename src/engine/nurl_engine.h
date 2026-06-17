#ifndef NURL_ENGINE_H
#define NURL_ENGINE_H

#include "engine/request.h"
#include "nurl_http.h"

typedef struct {
    double connect_time_sec;
    int    num_redirects;
} NurlOperationStats;

int nurl_engine_execute_request(
    NurlRequest *req,
    nurl_http_response_t **out_response,
    char **out_effective_url,
    NurlOperationStats *out_stats
);

#endif /* NURL_ENGINE_H */
