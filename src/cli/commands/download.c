#include "commands.h"
#include "nurl_net.h"
#include "nurl_tls.h"
#include "nurl_utils.h"
#include "nurl_http.h"
#include "nurl_engine.h"
#include "request.h"
#include "errors/nurl_error_handler.h"
#include "errors/nurl_diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>

#include "nurl_progress.h"

int nurl_cmd_download(const char *url, const CommonArgs *common) {
    char *scheme = NULL, *host = NULL, *path = NULL;
    int port = 0;

    // We still need to parse once to get the path for default filename, 
    // but we don't need the diagnostic block here since the engine handles it.
    if (nurl_utils_parse_url(url, &scheme, &host, &port, &path) != 0) {
        return NURL_ERR_INVALID_URL;
    }

    char *filename = NULL;
    if (common->output) {
        filename = strdup(common->output);
    } else {
        char *last_slash = strrchr(path, '/');
        if (last_slash && strlen(last_slash) > 1) {
            char *q = strchr(last_slash + 1, '?');
            if (q) *q = '\0';
            filename = strdup(last_slash + 1);
            if (q) *q = '?';
        } else {
            filename = strdup("download");
        }
    }

    if (!filename) {
        free(scheme); free(host); free(path);
        return NURL_ERR_OOM;
    }

    bool is_stdout = (strcmp(filename, "-") == 0);
    unsigned long start_pos = 0;
    if (common->resume && !is_stdout) {
        struct stat st;
        if (stat(filename, &st) == 0 && S_ISREG(st.st_mode)) {
            start_pos = (unsigned long)st.st_size;
        }
    }

    NurlRequest *req = nurl_request_new();
    if (!req) { free(filename); free(scheme); free(host); free(path); return NURL_ERR_OOM; }
    nurl_request_from_args(req, "GET", url, common);

    NurlProgressCtx p_ctx;
    if (common->progress) {
        p_ctx.resume_offset = 0;
        p_ctx.silent = common->silent;
        gettimeofday(&p_ctx.start_time, NULL);
        p_ctx.last_update = p_ctx.start_time;
        req->progress_cb = nurl_progress_update;
        req->progress_data = &p_ctx;
    }

    if (!common->silent) {
        fprintf(stderr, "* Saving to: %s\n", filename);
        if (start_pos > 0) nurl_diag_hint("resuming download from offset: %lu", start_pos);
    }

    unsigned int max_retries = common->retry;
    unsigned long delay_sec = common->retry_delay > 0 ? common->retry_delay : 1;
    int engine_err = NURL_OK;
    nurl_http_response_t *res = NULL;
    char *effective_url = NULL;

    for (unsigned int attempt = 0; attempt <= max_retries; attempt++) {
        if (res) { nurl_http_response_free(res); res = NULL; }
        if (effective_url) { free(effective_url); effective_url = NULL; }

        FILE *out = is_stdout ? stdout : fopen(filename, (start_pos > 0) ? "ab" : "wb");
        if (!out) {
            nurl_diag_err("could not open local file '%s' for writing.", filename);
            nurl_diag_hint("check your file permissions or verify that the directory exists.");
            nurl_request_free(req); free(filename); free(scheme); free(host); free(path);
            return NURL_ERR_WRITE;
        }

        req->out = out;
        req->resume_offset = start_pos;
        if (common->progress) p_ctx.resume_offset = start_pos;

        engine_err = nurl_engine_execute_request(req, &res, &effective_url);
        if (!is_stdout) fclose(out);

        if (engine_err == NURL_OK && res) {
            if (res->status_code < 500) break;
            if (attempt < max_retries && !common->silent) fprintf(stderr, "nurl: Warning: HTTP %d. Retrying...\n", res->status_code);
        } else {
            if (attempt < max_retries && !common->silent) fprintf(stderr, "nurl: Warning: Request failed (error %d). Retrying...\n", engine_err);
        }
        if (attempt < max_retries) sleep(delay_sec);
    }

    if (engine_err != NURL_OK) {
        if (!common->silent) nurl_handle_request_error(engine_err, req, effective_url ? effective_url : url);
    } else if (res && !common->silent && res->status_code >= 400) {
        nurl_handle_request_error(res->status_code >= 500 ? NURL_ERR_STATUS_5XX : NURL_ERR_STATUS_4XX, req, effective_url ? effective_url : url);
    }

    nurl_request_free(req);
    if (res) nurl_http_response_free(res);
    if (effective_url) free(effective_url);
    free(filename); free(scheme); free(host); free(path);
    return engine_err;
}
