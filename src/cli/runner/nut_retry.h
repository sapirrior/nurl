#ifndef NUT_RETRY_H
#define NUT_RETRY_H

#include "nut.h"
#include "engine/nut_engine_request.h"
#include "nut_engine.h"

/**
 * Executes a request with automatic retries on transient errors.
 */
int execute_with_retry(NutCtx *ctx, NutRequest *req, const CommonArgs *common, nut_http_response_t **out_res, char **out_effective_url, NutOperationStats *stats);

#endif /* NUT_RETRY_H */
