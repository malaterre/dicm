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
#include "dicm-writer.h"

#include "dicm-io.h"
#include "dicm-private.h"
#include "dicm-public.h"

#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct _dicm {
  struct dicm_writer writer;
};

/* object */
static DICM_CHECK_RETURN int _dicm_destroy(void *self_) DICM_NONNULL;

/* writer */
static DICM_CHECK_RETURN int _dicm_write_start_attribute(
    void *self, const struct dicm_attribute *da) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_end_attribute(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_value(void *self, const void *buf,
                                               size_t s) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_start_fragment(
    void *self, int frag_num) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_end_fragment(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_start_item(void *self,
                                                    int item_num) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_end_item(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_start_sequence(
    void *self, const struct dicm_attribute *da) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_end_sequence(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_start_model(
    void *self, const char *encoding) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_write_end_model(void *self) DICM_NONNULL;

static struct writer_vtable const g_vtable =
    {/* object interface */
     .object = {.fp_destroy = _dicm_destroy},
     /* writer interface */
     .writer = {
         .fp_write_start_attribute = _dicm_write_start_attribute,
         .fp_write_end_attribute = _dicm_write_end_attribute,
         .fp_write_value = _dicm_write_value,
         .fp_write_start_fragment = _dicm_write_start_fragment,
         .fp_write_end_fragment = _dicm_write_end_fragment,
         .fp_write_start_item = _dicm_write_start_item,
         .fp_write_end_item = _dicm_write_end_item,
         .fp_write_start_sequence = _dicm_write_start_sequence,
         .fp_write_end_sequence = _dicm_write_end_sequence,
         .fp_write_start_model = _dicm_write_start_model,
         .fp_write_end_model = _dicm_write_end_model,
     }};

int dicm_writer_utf8_create(struct dicm_writer **pself, struct dicm_io *dst) {
  struct _dicm *self = (struct _dicm *)malloc(sizeof(*self));
  if (self) {
    *pself = &self->writer;
    self->writer.vtable = &g_vtable;
    self->writer.dst = dst;
    return 0;
  }
  return 1;
}

/* object */
int _dicm_destroy(void *self_) {
  struct _dicm *self = (struct _dicm *)self_;
  free(self);
  return 0;
}

int _dicm_write_start_attribute_impl(struct _dicm *self,
                                     const struct dicm_attribute *da) {
  struct dicm_io *dst = self->writer.dst;
  union _ude ude;
  const bool is_vr16 = _ude_init(&ude, da);
  const size_t len = is_vr16 ? 8u : 12u;
  io_ssize err = dicm_io_write(dst, &ude, len);
  assert(err == (io_ssize)len);
  return 0;
}

/* writer */
int _dicm_write_start_attribute(void *self_, const struct dicm_attribute *da) {
  return _dicm_write_start_attribute_impl(self_, da);
}

int _dicm_write_end_attribute(void *self_) { return 0; }

int _dicm_write_value(void *self_, const void *buf, size_t s) {
  struct _dicm *self = (struct _dicm *)self_;
  struct dicm_io *dst = self->writer.dst;
  io_ssize err = dicm_io_write(dst, buf, s);
  assert(err == (io_ssize)s);

  return 0;
}
int _dicm_write_start_fragment(void *self_, int frag_num) {
  struct _dicm *self = (struct _dicm *)self_;
  struct dicm_io *dst = self->writer.dst;
  union _ude ude;
  _ide_set_tag(&ude, TAG_STARTITEM);
  _ide_set_vl(&ude, VL_UNDEFINED);
  io_ssize err = dicm_io_write(dst, &ude.ide, 8);
  assert(err == 8);

  return 0;
}
int _dicm_write_end_fragment(void *self_) { return 0; }
int _dicm_write_start_item(void *self_, int item_num) {
  struct _dicm *self = (struct _dicm *)self_;
  struct dicm_io *dst = self->writer.dst;
  union _ude ude;
  _ide_set_tag(&ude, TAG_STARTITEM);
  _ide_set_vl(&ude, VL_UNDEFINED);
  io_ssize err = dicm_io_write(dst, &ude.ide, 8);
  assert(err == 8);

  return 0;
}
int _dicm_write_end_item(void *self_) {
  struct _dicm *self = (struct _dicm *)self_;
  struct dicm_io *dst = self->writer.dst;
  union _ude ude;
  _ide_set_tag(&ude, TAG_ENDITEM);
  _ide_set_vl(&ude, 0);

  io_ssize err = dicm_io_write(dst, &ude.ide, 8);
  assert(err == 8);

  return 0;
}
int _dicm_write_start_sequence(void *self_, const struct dicm_attribute *da) {
  return _dicm_write_start_attribute_impl(self_, da);
}
int _dicm_write_end_sequence(void *self_) {
  struct _dicm *self = (struct _dicm *)self_;
  struct dicm_io *dst = self->writer.dst;
  union _ude ude;
  _ide_set_tag(&ude, TAG_ENDSQITEM);
  _ide_set_vl(&ude, 0);
  io_ssize err = dicm_io_write(dst, &ude.ide, 8);
  assert(err == 8);

  return 0;
}
int _dicm_write_start_model(void *self_, const char *encoding) { return 0; }
int _dicm_write_end_model(void *self_) { return 0; }
