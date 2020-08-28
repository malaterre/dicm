#include "dicm-log.h"

#include <stdio.h>
// dummy logger

struct _log *dlog_init(struct _mem* mem, const char *fspec) {}
void dlog_trace(struct _log *log, const char *msg) {}
void dlog_debug(struct _log *log, const char *msg) {
  fprintf(stderr, "DEBUG: %s\n", msg);
}
void dlog_info(struct _log *log, const char *msg) {}
void dlog_warn(struct _log *log, const char *msg) {}
void dlog_error(struct _log *log, const char *msg) {}
void dlog_fatal(struct _log *log, const char *msg) {}
int dlog_fini(struct _log *log)
{
}

static const struct _log_ops dlog_ops = {
  .init = dlog_init,
  .trace = dlog_trace,
  .debug = dlog_debug,
  .info = dlog_info,
  .warn = dlog_warn,
  .error = dlog_error,
  .fatal = dlog_fatal,
  .fini = dlog_fini,
};

struct _log dlog = {
  .ops = &dlog_ops
};

