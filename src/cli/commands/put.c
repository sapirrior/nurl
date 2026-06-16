#include "commands.h"
#include "nurl_request.h"

int nurl_cmd_put(const char *url, const CommonArgs *common) {
    return nurl_request_generic("PUT", url, common);
}
