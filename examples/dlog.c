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
#include <stdlib.h>

struct _log {
  struct dicm_log log;
  /* data */
  FILE *stream;
};

static DICM_CHECK_RETURN int _log_destroy(void *self_) DICM_NONNULL;
static DICM_CHECK_RETURN int _log_msg(void *self_, int /*log_level_t*/ llevel,
                                      const char *str) DICM_NONNULL;

static struct log_vtable const g_vtable = {
    /* object interface */
    .object = {.fp_destroy = _log_destroy},
    /* log interface */
    .log = {.fp_msg = _log_msg}};

int dicm_log_create(struct dicm_log **pself, FILE *stream) {
  struct _log *self = (struct _log *)malloc(sizeof(*self));
  if (self) {
    *pself = &self->log;
    self->log.vtable = &g_vtable;
    self->stream = stream;
    return 0;
  }
  *pself = NULL;
  return 1;
}

int _log_destroy(void *const self_) {
  struct _file *self = (struct _file *)self_;
  free(self);
  return 0;
}

static const char *strlevels[] = {"TRACE", "DEBUG", "INFO",
                                  "WARN",  "ERROR", "FATAL"};

int _log_msg(void *const self_, int /*log_level_t*/ llevel, const char *str) {
  struct _log *self = (struct _log *)self_;
  fprintf(self->stream, "%s: %s\n", strlevels[llevel], str);
  return 0;
}
