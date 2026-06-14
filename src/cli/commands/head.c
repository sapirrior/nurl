#include "commands.h"
#include "nurl_request.h"

int nurl_cmd_head(const char *url, const CommonArgs *common) {
    // For HEAD command, we always want to include headers in the output.
    CommonArgs args_copy = *common;
    args_copy.include = true;
    return nurl_request_generic("HEAD", url, &args_copy);
}
