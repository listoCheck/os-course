#ifndef VTFS_HTTP_H
#define VTFS_HTTP_H

#include <linux/types.h>

int64_t vtfs_http_call(const char *token, const char *method,
                      char *response_buffer, size_t buffer_size,
                      size_t arg_size, ...);

void encode(const char *src, char *dst);

#endif // VTFS_HTTP_H