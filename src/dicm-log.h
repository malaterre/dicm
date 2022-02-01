#pragma once

#include "dicm-features.h"

struct _log {
  const struct _log_ops *ops;
  void *data;
};

typedef enum { trace = 0, debug, info, warn, error, fatal } log_level_t;

struct _log_ops {
  struct _log *(*init)(struct _mem *mem, const char *fspec);
  void (*msg)(struct _log *log, log_level_t llevel, const char *msg);
  //  void (*trace)(struct _log *log, const char *msg);
  //  void (*debug)(struct _log *log, const char *msg);
  //  void (*info)(struct _log *log, const char *msg);
  //  void (*warn)(struct _log *log, const char *msg);
  //  void (*error)(struct _log *log, const char *msg);
  //  void (*fatal)(struct _log *log, const char *msg);
  int (*fini)(struct _log *log);
};

DICM_EXPORT void log_errno(log_level_t llevel, int errnum);

DICM_EXPORT void set_global_logger(struct _log *log);

extern struct _log *global_log;
