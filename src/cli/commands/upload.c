#include "commands.h"
#include "nurl_engine.h"
#include "nurl_utils.h"
#include "errors/nurl_error_handler.h"
#include "errors/nurl_diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int nurl_cmd_upload(const char *url, const CommonArgs *common) {
    if (!common->upload_file) {
        nurl_diag_err("no upload file specified.");
        nurl_diag_hint("use --upload <file> to specify the target.");
        return NURL_ERR_GENERIC;
    }

    struct stat st;
    if (stat(common->upload_file, &st) != 0 || !S_ISREG(st.st_mode)) {
        nurl_diag_err("could not read upload file '%s'.", common->upload_file);
        return NURL_ERR_IO;
    }

    nurl_http_response_t *res = NULL;
    char *effective_url = NULL;

    NurlRequest *req = nurl_request_new();
    if (!req) return NURL_ERR_OOM;
    nurl_request_from_args(req, "POST", url, common);

    nurl_err_t err = nurl_engine_execute_request(req, &res, &effective_url);
    if (err != NURL_OK) {
        if (!common->silent) nurl_handle_request_error(err, req, effective_url ? effective_url : url);
    } else if (res && !common->silent && res->status_code >= 400) {
        nurl_handle_request_error(res->status_code >= 500 ? NURL_ERR_STATUS_5XX : NURL_ERR_STATUS_4XX, req, effective_url ? effective_url : url);
    }

    if (err == NURL_OK && res && !common->silent && (res->status_code < 400 || !common->fail)) {
        if (res->body_len > 0) fwrite(res->body, 1, res->body_len, stdout);
    }

    int ret = (err != NURL_OK) ? err : (res && res->status_code >= 400 ? (res->status_code >= 500 ? NURL_ERR_STATUS_5XX : NURL_ERR_STATUS_4XX) : NURL_OK);
    nurl_request_free(req);
    if (res) nurl_http_response_free(res);
    if (effective_url) free(effective_url);
    return ret;
}
