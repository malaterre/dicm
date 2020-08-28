#include "dicm-log.h"

#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <string.h>

struct _log *global_log = NULL;

//struct _log *get_global_logger();
void set_global_logger(struct _log *log)
{
  global_log = log;
}

void log_errno()
{
  char buf[1024];
  strerror_r(errno, buf, sizeof buf);
  global_log->ops->debug(global_log, buf);
}
