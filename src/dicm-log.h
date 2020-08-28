#pragma once

#include "dicm-mem.h"

struct _log {
  const struct _log_ops *ops;
  void *data;
};

struct _log_ops {
  struct _log *(*init)(struct _mem* mem, const char *fspec);
  void (*trace)(struct _log *log, const char *msg);
  void (*debug)(struct _log *log, const char *msg);
  void (*info)(struct _log *log, const char *msg);
  void (*warn)(struct _log *log, const char *msg);
  void (*error)(struct _log *log, const char *msg);
  void (*fatal)(struct _log *log, const char *msg);
  int (*fini)(struct _log *log);
};

