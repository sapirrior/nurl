#ifndef NURL_REQUEST_BUILD_H
#define NURL_REQUEST_BUILD_H

#include "utils/nurl_headers.h"
#include "nurl.h"

nurl_err_t nurl_headermap_apply_auth(NurlHeaderMap *m, const CommonArgs *a);
nurl_err_t nurl_headermap_apply_common(NurlHeaderMap *m, const CommonArgs *a);

#endif /* NURL_REQUEST_BUILD_H */
