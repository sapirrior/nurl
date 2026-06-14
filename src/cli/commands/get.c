#include "commands.h"
#include "nurl_request.h"

int nurl_cmd_get(const char *url, const CommonArgs *common) {
    return nurl_request_generic("GET", url, common);
}
