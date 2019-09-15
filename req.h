#ifndef _REQ_H_
#define _REQ_H_

#include "cJSON.h"

#define REQ_USERAGENT "libcurl-agent/1.0"

typedef struct {
	const char ** headers;
} req_options;

typedef struct {
  char * memory;
  size_t size;
} req_mem;

cJSON *
req_get(const char *, req_options *, long *);

cJSON *
req_put(const char *, cJSON *, req_options *, long *);

cJSON *
req_post(const char *, cJSON *, req_options *, long *);

#endif /* !_REQ_H_ */

