/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-public.h"

typedef enum { trace = 0, debug, info, warn, error, fatal } log_level_t;

struct log_prv_vtable {
  DICM_CHECK_RETURN int (*fp_msg)(void *const, int /*log_level_t*/ llevel,
                                  const char *msg) DICM_NONNULL;
};

/* common log vtable */
struct log_vtable {
  struct object_prv_vtable const object;
  struct log_prv_vtable const log;
};

/* common log object */
struct dicm_log {
  struct log_vtable const *vtable;
};

DICM_EXPORT void dicm_log_set_global(struct dicm_log *log);
DICM_EXPORT DICM_CHECK_RETURN struct dicm_log *dicm_log_get_global(void);

/* common log interface */
static inline void dicm_log_msg(int /*log_level_t*/ log_level,
                                const char *str) {
  const struct dicm_log *log = dicm_log_get_global();
  if (log) log->vtable->log.fp_msg(log, log_level, str);
}
