#ifndef NUT_PROGRESS_H
#define NUT_PROGRESS_H

#include "engine/nut_engine_request.h"
#include <stdbool.h>

typedef struct {
    unsigned long resume_offset;
    double start_time;
    double last_update;
    bool silent;
} NutProgressCtx;

/**
 * The actual callback implementation that draws to the terminal.
 */
void nut_progress_update(unsigned long downloaded, unsigned long total, bool finished, void *user_data);

#endif /* NUT_PROGRESS_H */
