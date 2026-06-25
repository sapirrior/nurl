#ifndef NUT_MULTIPART_H
#define NUT_MULTIPART_H

#include "engine/nut_engine_request.h"

typedef struct NutMultipart NutMultipart;

NutMultipart *nut_multipart_new(void);
void nut_multipart_add_file(NutMultipart *m, const char *field_name,
                             const char *filepath, const char *mime_type);
void nut_multipart_add_field(NutMultipart *m, const char *name, const char *value);

// Returns Content-Type header value ("multipart/form-data; boundary=...")
const char *nut_multipart_content_type(const NutMultipart *m);

void nut_multipart_into_request(NutMultipart *m, NutRequest *req);
void nut_multipart_free(NutMultipart *m);

#endif /* NUT_MULTIPART_H */
