#ifndef NUT_ERROR_HANDLER_H
#define NUT_ERROR_HANDLER_H

#include "nut_error.h"
#include "engine/nut_engine_request.h"

/**
 * Centrally handles request errors by emitting smart, context-aware diagnostics.
 */
void nut_handle_request_error(nut_err_t err, const NutRequest *req, const char *target_url);

#endif /* NUT_ERROR_HANDLER_H */
