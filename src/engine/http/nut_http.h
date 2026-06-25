#ifndef NUT_HTTP_H
#define NUT_HTTP_H

#include "net/nut_stream.h"
#include "engine/nut_engine_types.h"
#include "errors/nut_error.h"
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

/**
 * Sends a structured HTTP/1.1 request via the provided active buffered stream.
 * method: "GET", "POST", etc.
 * path: The request path and query parameters (e.g. "/json?q=1").
 * hostname: The destination Host header value.
 * extra_headers: Null-terminated string containing extra "Name: Value\r\n" headers, or NULL.
 * body: Payload bytes, or NULL.
 * body_len: Length of payload in bytes.
 * Returns NUT_OK on success, or an explicit nut_err_t on failure.
 * If successful, *out_response will contain the dynamically allocated response.
 */

typedef struct NutHttpParams NutHttpParams;
typedef void (*nut_headers_cb)(NutHttpParams *p, const nut_http_response_t *res, void *user_data);

struct NutHttpParams {
    const char        *method;
    const char        *path;
    const char        *hostname;
    const char        *extra_headers;
    const uint8_t     *body;
    size_t             body_len;
    NutBodyPart      *body_parts;
    size_t             body_parts_count;
    FILE              *body_out;
    unsigned long      resume_offset;
    nut_progress_cb   progress_cb;
    void              *progress_data;
    nut_headers_cb    header_cb;
    void              *header_data;
    bool               http10;
};

nut_err_t nut_http_request(
    NutStream *stream,
    NutHttpParams *p,
    nut_http_response_t **out_response
);

/**
 * Frees all memory associated with the response structure.
 */
void nut_http_response_free(nut_http_response_t *res);

#endif /* NUT_HTTP_H */
