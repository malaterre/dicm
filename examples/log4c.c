#include "dicm-log.h"

struct _log *(*log4c_init)(struct _mem* mem, const char *fspec) {}
void log4c_trace(struct _log *log, const char *msg) {}
void log4c_debug(struct _log *log, const char *msg) {}
void log4c_info(struct _log *log, const char *msg) {}
void log4c_warn(struct _log *log, const char *msg) {}
void log4c_error(struct _log *log, const char *msg) {}
void log4c_fatal(struct _log *log, const char *msg) {}
int log4c_fini(struct _log *log)
{
}

static const struct _log_ops log4c_ops = {
  .init = log4c_init;
  .trace = log4c_trace;
  .debug = log4c_debug;
  .info = log4c_info;
  .warn = log4c_warn;
  .error = log4c_error;
  .fatal = log4c_fatal;
  .fini = log4c_fini;
};
