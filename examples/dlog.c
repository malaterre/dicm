/*
 *  DICM, a library for reading DICOM instances
 *
 *  Copyright (c) 2020 Mathieu Malaterre
 *  All rights reserved.
 *
 *  DICM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, version 2.1.
 *
 *  DICM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with DICM . If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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

