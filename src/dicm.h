#pragma once

#include "dicm-features.h"
#include "dicm-private.h"

int dicm_sreader_init(struct _dicm_sreader *sreader, struct _src *src);
int dicm_sreader_hasnext(struct _dicm_sreader *sreader);
int dicm_sreader_next(struct _dicm_sreader *sreader);
int dicm_sreader_fini(struct _dicm_sreader *sreader);

// typedef struct _dicm dicm_t;
typedef struct _dicm_sreader dicm_sreader_t;

typedef struct _src src_t;
typedef struct _dst dst_t;
