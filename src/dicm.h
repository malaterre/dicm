#pragma once

#include "dicm-features.h"
#include "dicm-private.h"

enum error {
  kSuccess = 0,
  kError = -1
};

int dicm_sreader_init(struct _dicm_sreader *sreader, struct _src *src);
int dicm_sreader_hasnext(struct _dicm_sreader *sreader);
int dicm_sreader_next(struct _dicm_sreader *sreader);
int dicm_sreader_fini(struct _dicm_sreader *sreader);

struct _dataelement;
int dicm_sreader_get_dataelement(struct _dicm_sreader *sreader, struct _dataelement *de);

// typedef struct _dicm dicm_t;
typedef struct _dicm_sreader dicm_sreader_t;

typedef struct _src src_t;
typedef struct _dst dst_t;
