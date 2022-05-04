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

#pragma once

#include "dicm-features.h"
#include "dicm-public.h"

#include <stddef.h> /* size_t */

struct writer_prv_vtable {
  /* kAttribute */
  int (*fp_write_attribute)(void *const, const struct dicm_attribute *);

  /* kValue: valid for both attribute and fragment */
  int (*fp_write_value_length)(void *const, size_t);
  int (*fp_write_value)(void *const, const void *, size_t);

  /* kFragment */
  int (*fp_write_fragment)(void *const);

  /* kItem */
  int (*fp_write_start_item)(void *const);
  int (*fp_write_end_item)(void *const);

  /* kSequence: valid for SQ and Pixel Data,OB,u/l */
  int (*fp_write_start_sequence)(void *const);
  int (*fp_write_end_sequence)(void *const);

  /* We need a start model to implement easy conversion to XML */
  int (*fp_write_start_dataset)(void *const, const char *);
  int (*fp_write_end_dataset)(void *const);
};

/* common writer vtable */
struct writer_vtable {
  struct object_prv_vtable const object;
  struct writer_prv_vtable const writer;
};

/* common writer object */
struct dicm_writer {
  struct writer_vtable const *vtable;
  struct dicm_io *dst;
};

/* common writer interface */
#define dicm_writer_write_attribute(t, da) \
  ((t)->vtable->writer.fp_write_attribute((t), (da)))
#define dicm_writer_write_value_length(t, s) \
  ((t)->vtable->writer.fp_write_value_length((t), (s)))
#define dicm_writer_write_value(t, b, s) \
  ((t)->vtable->writer.fp_write_value((t), (b), (s)))
#define dicm_writer_write_fragment(t) \
  ((t)->vtable->writer.fp_write_fragment((t)))
#define dicm_writer_write_start_item(t) \
  ((t)->vtable->writer.fp_write_start_item((t)))
#define dicm_writer_write_end_item(t) \
  ((t)->vtable->writer.fp_write_end_item((t)))
#define dicm_writer_write_start_sequence(t) \
  ((t)->vtable->writer.fp_write_start_sequence((t)))
#define dicm_writer_write_end_sequence(t) \
  ((t)->vtable->writer.fp_write_end_sequence((t)))
#define dicm_writer_write_start_dataset(t, m) \
  ((t)->vtable->writer.fp_write_start_dataset((t), (m)))
#define dicm_writer_write_end_dataset(t) \
  ((t)->vtable->writer.fp_write_end_dataset((t)))

DICM_EXPORT int dicm_writer_utf8_create(struct dicm_writer **pself,
                                        struct dicm_io *dst);
