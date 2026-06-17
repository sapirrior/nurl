#ifndef NURL_DISPATCH_H
#define NURL_DISPATCH_H

#include "nurl.h"
#include "request.h"
#include "nurl_engine.h"

int execute_with_retry(NurlRequest *req, const CommonArgs *common, nurl_http_response_t **out_res, char **out_effective_url, NurlOperationStats *stats);
int nurl_dispatch(const char *method, const char *url, const CommonArgs *args);

#endif /* NURL_DISPATCH_H */
