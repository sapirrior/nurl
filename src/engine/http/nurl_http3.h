#ifndef NURL_HTTP3_H
#define NURL_HTTP3_H

#include "nurl_http.h"
#include "engine/request.h"
#include <stdio.h>

/**
 * Executes a request using HTTP/3 protocol over UDP.
 */
nurl_http_response_t *nurl_http3_request(
    const char *method,
    const char *path,
    const char *hostname,
    int port,
    const char *extra_headers,
    const unsigned char *body,
    size_t body_len,
    NurlBodyPart *body_parts,
    size_t body_parts_count,
    FILE *body_out,
    bool show_progress,
    bool silent,
    unsigned long resume_offset,
    bool tls_verify,
    const char *cacert,
    const char *cert,
    const char *key
);

#endif /* NURL_HTTP3_H */
