#include "commands.h"
#include "nurl_engine.h"
#include "nurl_http.h"
#include "errors/nurl_error_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int nurl_cmd_options(const char *url, const CommonArgs *common) {
    nurl_http_response_t *res = NULL;
    char *effective_url = NULL;

    NurlRequest *req = nurl_request_new();
    if (!req) return NURL_ERR_OOM;
    nurl_request_from_args(req, "OPTIONS", url, common);
    req->silent = true; // We'll handle output manually

    nurl_err_t err = nurl_engine_execute_request(req, &res, &effective_url);
    if (err != NURL_OK) {
        if (!common->silent) nurl_handle_request_error(err, req, effective_url ? effective_url : url);
        nurl_request_free(req);
        if (effective_url) free(effective_url);
        return err;
    }

    if (!common->silent && res) {
        for (size_t i = 0; i < res->header_count; i++) {
            if (strncasecmp(res->headers[i], "Allow:", 6) == 0 || strncasecmp(res->headers[i], "Access-Control-", 15) == 0) {
                printf("%s\n", res->headers[i]);
            }
        }
    }

    int ret = (res && res->status_code >= 400) ? (res->status_code >= 500 ? NURL_ERR_STATUS_5XX : NURL_ERR_STATUS_4XX) : NURL_OK;
    nurl_request_free(req);
    if (res) nurl_http_response_free(res);
    if (effective_url) free(effective_url);
    return ret;
}
