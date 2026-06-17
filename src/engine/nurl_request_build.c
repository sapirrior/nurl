#include "nurl_request_build.h"
#include "utils/nurl_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

nurl_err_t nurl_headermap_apply_auth(NurlHeaderMap *m, const CommonArgs *a) {
    if (!m || !a) return NURL_ERR_GENERIC;
    if (a->no_auth) return NURL_OK;

    if (nurl_headermap_has(m, "Authorization")) {
        return NURL_OK;
    }

    if (a->bearer || a->token) {
        const char *tok = a->bearer ? a->bearer : a->token;
        char auth_val[1024];
        snprintf(auth_val, sizeof(auth_val), "Bearer %s", tok);
        return nurl_headermap_set(m, "Authorization", auth_val);
    } else if (a->user) {
        char *b64 = nurl_utils_base64_encode((const unsigned char *)a->user, strlen(a->user));
        if (!b64) return NURL_ERR_OOM;
        char auth_val[1024];
        snprintf(auth_val, sizeof(auth_val), "Basic %s", b64);
        free(b64);
        return nurl_headermap_set(m, "Authorization", auth_val);
    }

    return NURL_OK;
}

nurl_err_t nurl_headermap_apply_common(NurlHeaderMap *m, const CommonArgs *a) {
    if (!m || !a) return NURL_ERR_GENERIC;

    if (a->user_agent && !nurl_headermap_has(m, "User-Agent")) {
        nurl_err_t err = nurl_headermap_set(m, "User-Agent", a->user_agent);
        if (err != NURL_OK) return err;
    }
    if (a->referer && !nurl_headermap_has(m, "Referer")) {
        nurl_err_t err = nurl_headermap_set(m, "Referer", a->referer);
        if (err != NURL_OK) return err;
    }
    if (a->cookie && !nurl_headermap_has(m, "Cookie")) {
        nurl_err_t err = nurl_headermap_set(m, "Cookie", a->cookie);
        if (err != NURL_OK) return err;
    }

    return NURL_OK;
}
