#include "commands.h"
#include "nurl_request.h"

int nurl_cmd_options(const char *url, const CommonArgs *common) {
    // For OPTIONS command, we also default to printing response headers to show CORS options.
    CommonArgs args_copy = *common;
    args_copy.include = true;
    return nurl_request_generic("OPTIONS", url, &args_copy);
}
