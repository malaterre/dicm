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

#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <string.h>

// https://stackoverflow.com/questions/17960423/what-is-the-oldest-version-of-gcc-glibc-that-supports-the-strerrorlen-s-and-stre
#ifdef _MSC_VER
#define strerror_r(errno, buf, len) strerror_s(buf, len, errno)
#endif

struct _log *global_log = NULL;

void set_global_logger(struct _log *log) { global_log = log; }

void log_errno(log_level_t llevel, int errnum) {
  char buf[1024];
  // FIXME: errno may have changed at this point...remove API!!!!
  strerror_r(errnum, buf, sizeof buf);
  global_log->ops->msg(global_log, llevel, buf);
}
