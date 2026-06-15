#include "nurl_http2.h"
#include <nghttp2/nghttp2.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef NURL_VERSION
#define NURL_VERSION "0.2.0"
#endif

typedef struct {
    nurl_tls_t *tls;
    nurl_http_response_t *res;
    FILE *body_out;
    bool show_progress;
    bool silent;
    unsigned long downloaded;
    unsigned long total_len;
    unsigned long resume_offset;
    struct timeval start_time;
    struct timeval last_update;
    
    // Send context
    const unsigned char *body;
    size_t body_len;
    size_t body_sent;
    NurlBodyPart *body_parts;
    size_t body_parts_count;
    size_t current_part;
    size_t current_part_sent;
    FILE *current_file;
    
    // Receiving headers building
    char **headers_buf;
    size_t headers_cap;
    size_t headers_count;
    
    bool got_status;
} http2_session_data;

static void update_progress_bar_h2(http2_session_data *data) {
    if (!data->show_progress || data->silent) return;
    struct timeval now;
    gettimeofday(&now, NULL);
    double elapsed_sec = (now.tv_sec - data->start_time.tv_sec) + (now.tv_usec - data->start_time.tv_usec) / 1000000.0;
    double since_last_sec = (now.tv_sec - data->last_update.tv_sec) + (now.tv_usec - data->last_update.tv_usec) / 1000000.0;

    if (since_last_sec >= 0.2 || (data->total_len > 0 && data->downloaded == data->total_len)) {
        data->last_update = now;
        double speed_mb = 0.0;
        if (elapsed_sec > 0.0) {
            speed_mb = ((double)(data->downloaded - data->resume_offset) / (1024.0 * 1024.0)) / elapsed_sec;
        }

        if (data->total_len > 0) {
            int percent = (int)(((double)data->downloaded / (double)data->total_len) * 100.0);
            double remaining_sec = 0.0;
            if (data->total_len > data->downloaded && speed_mb > 0.0) {
                remaining_sec = (double)(data->total_len - data->downloaded) / (speed_mb * 1024.0 * 1024.0);
            }
            fprintf(stderr, "\r  %.2f MB / %.2f MB  %d%%  %.2f MB/s  %.0fs left",
                (double)data->downloaded / (1024.0 * 1024.0),
                (double)data->total_len / (1024.0 * 1024.0),
                percent,
                speed_mb,
                remaining_sec);
        } else {
            fprintf(stderr, "\r  %.2f MB / Unknown  %.2f MB/s",
                (double)data->downloaded / (1024.0 * 1024.0),
                speed_mb);
        }
        fflush(stderr);
    }
}

// Callback called by nghttp2 when it wants to send data
static ssize_t send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data) {
    (void)session;
    (void)flags;
    http2_session_data *sd = (http2_session_data *)user_data;
    int n = nurl_tls_write(sd->tls, data, (int)length);
    if (n < 0) return NGHTTP2_ERR_CALLBACK_FAILURE;
    return n;
}

// Callback called by nghttp2 when it wants to read data
static ssize_t recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data) {
    (void)session;
    (void)flags;
    http2_session_data *sd = (http2_session_data *)user_data;
    int n = nurl_tls_read(sd->tls, buf, (int)length);
    if (n < 0) return NGHTTP2_ERR_CALLBACK_FAILURE;
    if (n == 0) return NGHTTP2_ERR_EOF;
    return n;
}

// Callback called when a frame is received
static int on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
    (void)session;
    (void)user_data;
    (void)frame;
    return 0;
}

// Callback called for each header name-value pair
static int on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data) {
    (void)session;
    (void)frame;
    (void)flags;
    http2_session_data *sd = (http2_session_data *)user_data;
    
    char name_s[512];
    char val_s[4096];
    
    if (namelen >= sizeof(name_s)) namelen = sizeof(name_s) - 1;
    memcpy(name_s, name, namelen);
    name_s[namelen] = '\0';
    
    if (valuelen >= sizeof(val_s)) valuelen = sizeof(val_s) - 1;
    memcpy(val_s, value, valuelen);
    val_s[valuelen] = '\0';

    if (strcmp(name_s, ":status") == 0) {
        sd->res->status_code = atoi(val_s);
        sd->res->status_text = strdup(val_s);
        sd->got_status = true;
    } else {
        // Build header line: "name: value"
        char header_line[5000];
        snprintf(header_line, sizeof(header_line), "%s: %s", name_s, val_s);
        
        if (sd->headers_count >= sd->headers_cap) {
            sd->headers_cap = (sd->headers_cap == 0) ? 16 : sd->headers_cap * 2;
            char **temp = realloc(sd->headers_buf, sizeof(char *) * sd->headers_cap);
            if (temp) sd->headers_buf = temp;
        }
        if (sd->headers_buf) {
            sd->headers_buf[sd->headers_count++] = strdup(header_line);
        }
        
        // Detect Content-Length
        if (strcasecmp(name_s, "content-length") == 0) {
            sd->total_len = strtoul(val_s, NULL, 10);
        }
    }
    return 0;
}

// Callback called when data chunk is received
static int on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data) {
    (void)session;
    (void)flags;
    (void)stream_id;
    http2_session_data *sd = (http2_session_data *)user_data;
    
    if (sd->body_out) {
        fwrite(data, 1, len, sd->body_out);
    } else {
        unsigned char *temp = realloc(sd->res->body, sd->res->body_len + len + 1);
        if (temp) {
            sd->res->body = temp;
            memcpy(sd->res->body + sd->res->body_len, data, len);
            sd->res->body_len += len;
            sd->res->body[sd->res->body_len] = '\0';
        }
    }
    
    sd->downloaded += len;
    update_progress_bar_h2(sd);
    return 0;
}

// Data provider callback for HTTP/2 POST body streaming
static ssize_t data_provider_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data) {
    (void)session;
    (void)stream_id;
    (void)source;
    http2_session_data *sd = (http2_session_data *)user_data;
    
    size_t to_write = 0;
    
    if (sd->body_parts && sd->body_parts_count > 0) {
        // Multipart or chunked parts
        while (sd->current_part < sd->body_parts_count) {
            NurlBodyPart *part = &sd->body_parts[sd->current_part];
            if (part->type == NURL_BODY_PART_MEM) {
                size_t avail = part->len - sd->current_part_sent;
                if (avail > 0) {
                    to_write = avail < length ? avail : length;
                    memcpy(buf, part->data + sd->current_part_sent, to_write);
                    sd->current_part_sent += to_write;
                    if (sd->current_part_sent == part->len) {
                        sd->current_part++;
                        sd->current_part_sent = 0;
                    }
                    return (ssize_t)to_write;
                }
            } else if (part->type == NURL_BODY_PART_FILE) {
                if (!sd->current_file && part->filepath) {
                    sd->current_file = fopen(part->filepath, "rb");
                    if (!sd->current_file) return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
                }
                if (sd->current_file) {
                    size_t r = fread(buf, 1, length, sd->current_file);
                    if (r > 0) {
                        return (ssize_t)r;
                    } else {
                        fclose(sd->current_file);
                        sd->current_file = NULL;
                        sd->current_part++;
                        sd->current_part_sent = 0;
                    }
                }
            }
        }
    } else if (sd->body && sd->body_len > 0) {
        size_t avail = sd->body_len - sd->body_sent;
        if (avail > 0) {
            to_write = avail < length ? avail : length;
            memcpy(buf, sd->body + sd->body_sent, to_write);
            sd->body_sent += to_write;
            if (sd->body_sent == sd->body_len) {
                *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            }
            return (ssize_t)to_write;
        }
    }
    
    *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    return 0;
}

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
) {
    http2_session_data sd;
    memset(&sd, 0, sizeof(sd));
    sd.tls = tls;
    sd.body_out = body_out;
    sd.show_progress = show_progress;
    sd.silent = silent;
    sd.resume_offset = resume_offset;
    sd.downloaded = resume_offset;
    gettimeofday(&sd.start_time, NULL);
    sd.last_update = sd.start_time;
    
    sd.body = body;
    sd.body_len = body_len;
    sd.body_parts = body_parts;
    sd.body_parts_count = body_parts_count;
    
    sd.res = calloc(1, sizeof(nurl_http_response_t));
    if (!sd.res) return NULL;
    
    nghttp2_session_callbacks *callbacks;
    nghttp2_session_callbacks_new(&callbacks);
    nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);
    nghttp2_session_callbacks_set_recv_callback(callbacks, recv_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
    
    nghttp2_session *session;
    nghttp2_session_client_new(&session, callbacks, &sd);
    nghttp2_session_callbacks_del(callbacks);
    
    // Submit settings frame
    nghttp2_settings_entry iv[1] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}
    };
    nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, iv, 1);
    
    // Assemble headers
    size_t h_cap = 16 + (extra_headers ? 10 : 0);
    nghttp2_nv *nva = malloc(sizeof(nghttp2_nv) * h_cap);
    size_t nv_count = 0;
    
    #define ADD_HEADER(N, V) { \
        nva[nv_count].name = (uint8_t *)(N); \
        nva[nv_count].namelen = strlen(N); \
        nva[nv_count].value = (uint8_t *)(V); \
        nva[nv_count].valuelen = strlen(V); \
        nva[nv_count].flags = NGHTTP2_NV_FLAG_NONE; \
        nv_count++; \
    }
    
    ADD_HEADER(":method", method);
    ADD_HEADER(":path", path);
    ADD_HEADER(":scheme", "https");
    ADD_HEADER(":authority", hostname);
    
    bool has_user_agent = false;
    
    if (extra_headers) {
        char *hdr_copy = strdup(extra_headers);
        char *line = strtok(hdr_copy, "\r\n");
        while (line) {
            char *colon = strchr(line, ':');
            if (colon) {
                *colon = '\0';
                char *key = line;
                char *val = colon + 1;
                while (*val && isspace((unsigned char)*val)) val++;
                
                // Lowercase the key (required by HTTP/2 specification)
                for (char *p = key; *p; p++) *p = tolower((unsigned char)*p);
                
                if (strcmp(key, "user-agent") == 0) has_user_agent = true;
                
                if (strcmp(key, "connection") != 0 && strcmp(key, "host") != 0) {
                    if (nv_count >= h_cap) {
                        h_cap *= 2;
                        nva = realloc(nva, sizeof(nghttp2_nv) * h_cap);
                    }
                    // We need to duplicate key and val because strtok mutates hdr_copy
                    nva[nv_count].name = (uint8_t *)strdup(key);
                    nva[nv_count].namelen = strlen(key);
                    nva[nv_count].value = (uint8_t *)strdup(val);
                    nva[nv_count].valuelen = strlen(val);
                    nva[nv_count].flags = NGHTTP2_NV_FLAG_NONE;
                    nv_count++;
                }
            }
            line = strtok(NULL, "\r\n");
        }
        free(hdr_copy);
    }
    
    if (!has_user_agent) {
        ADD_HEADER("user-agent", "nurl/" NURL_VERSION);
    }
    
    nghttp2_data_provider provider;
    nghttp2_data_provider *prov_ptr = NULL;
    
    bool has_body = (body && body_len > 0) || (body_parts && body_parts_count > 0);
    if (has_body) {
        provider.read_callback = data_provider_callback;
        provider.source.ptr = NULL;
        prov_ptr = &provider;
    }
    
    nghttp2_submit_request(session, NULL, nva, nv_count, prov_ptr, NULL);
    
    // Send session connection header (magic) and frames
    nghttp2_session_send(session);
    
    // Main read-write loop
    while (nghttp2_session_want_read(session) || nghttp2_session_want_write(session)) {
        int r = nghttp2_session_send(session);
        if (r != 0) break;
        
        r = nghttp2_session_recv(session);
        if (r != 0) break;
    }
    
    // Cleanup headers we allocated
    for (size_t i = 4; i < nv_count; i++) {
        // Method, path, scheme, authority and user-agent might be static literals, check if we duplicated them
        if (strcmp((char *)nva[i].name, "user-agent") != 0 || strcmp((char *)nva[i].name, "user-agent") != 0) {
            free(nva[i].name);
            free(nva[i].value);
        }
    }
    free(nva);
    nghttp2_session_del(session);
    
    if (sd.current_file) fclose(sd.current_file);
    
    if (sd.headers_count > 0) {
        sd.res->headers = sd.headers_buf;
        sd.res->header_count = sd.headers_count;
    } else {
        free(sd.headers_buf);
    }
    
    if (show_progress && !silent) {
        fprintf(stderr, "\n");
    }
    
    if (!sd.got_status) {
        nurl_http_response_free(sd.res);
        return NULL;
    }
    
    return sd.res;
}
