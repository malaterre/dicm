#pragma once

#include "dicm-features.h"
#include "dicm-io.h"

enum error {
  /** Return upon function success */
  kSuccess = 0,
  /** Generic error */
  kError = -1,
  /** DataElement have been sent in out of order */
  kOutOfOrder = -2,
  /** Value Representation if non-ASCII uppercase (A-Z only)*/
  kInvalidVR = -3
};

struct _dicm_sreader;
// int dicm_sreader_init(struct _dicm_sreader *sreader, struct _src *src);
struct _dicm_sreader *dicm_sreader_init(struct _src *src);
int dicm_sreader_hasnext(struct _dicm_sreader *sreader);
int dicm_sreader_next(struct _dicm_sreader *sreader);
int dicm_sreader_fini(struct _dicm_sreader *sreader);

struct _dataelement;
int dicm_sreader_get_dataelement(struct _dicm_sreader *sreader,
                                 struct _dataelement *de);

typedef struct _dicm_sreader dicm_sreader_t;

typedef struct _src src_t;
typedef struct _dst dst_t;
typedef struct _dataelement dataelement_t;
