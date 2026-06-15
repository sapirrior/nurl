#ifndef NURL_HTTP2_H
#define NURL_HTTP2_H

#include "nurl_tls.h"
#include "nurl_http.h"
#include "engine/request.h"
#include <stdio.h>

/**
 * Executes a request using HTTP/2 protocol over the negotiated TLS connection.
 */
nurl_http_response_t *nurl_http2_request(
    nurl_tls_t *tls,
    const char *method,
    const char *path,
    const char *hostname,
    const char *extra_headers,
    const unsigned char *body,
    size_t body_len,
    NurlBodyPart *body_parts,
    size_t body_parts_count,
    FILE *body_out,
    bool show_progress,
    bool silent,
    unsigned long resume_offset
);

#endif /* NURL_HTTP2_H */
