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

struct _log *global_log = NULL;

void set_global_logger(struct _log *log)
{
  global_log = log;
}

void log_errno(log_level_t llevel)
{
  char buf[1024];
  strerror_r(errno, buf, sizeof buf);
  global_log->ops->msg(global_log, llevel, buf);
}
