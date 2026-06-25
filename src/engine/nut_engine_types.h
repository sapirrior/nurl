#ifndef NUT_ENGINE_TYPES_H
#define NUT_ENGINE_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* HTTP Response structure */
typedef struct {
    int status_code;
    char *status_text;
    char **headers;
    size_t header_count;
    unsigned char *body;
    size_t body_len;
} nut_http_response_t;

/* Multipart/Body parts */
typedef enum {
    NUT_BODY_PART_MEM,
    NUT_BODY_PART_FILE
} NutBodyPartType;

typedef struct {
    NutBodyPartType type;
    const uint8_t *data;
    size_t len;
    const char *filepath;
} NutBodyPart;

/* Progress callback */
typedef void (*nut_progress_cb)(unsigned long downloaded, unsigned long total, bool finished, void *user_data);

#endif /* NUT_ENGINE_TYPES_H */
