#ifndef NUT_REQUEST_H
#define NUT_REQUEST_H

#include "nut.h"
#include "engine/nut_ctx.h"

/**
 * Performs a generic, secure HTTP/1.1 request (GET, POST, etc.) including redirections,
 * and outputs the response.
 */
int nut_request_generic(NutCtx *ctx, const char *method, const char *url, const CommonArgs *common);
void dump_headers_to_file(const char *filename, const nut_http_response_t *res);

#endif /* NUT_REQUEST_H */
