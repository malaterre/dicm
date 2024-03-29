/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-features.h"
#include "dicm-io.h"
#include "dicm-public.h"
#include <stdbool.h>
#include <stddef.h>

struct reader_prv_vtable {
  /* kAttribute */
  int (*fp_get_attribute)(void *const, struct dicm_attribute *);

  /* kValue: valid for both attribute and fragment */
  int (*fp_get_value_length)(void *const, size_t *);
  int (*fp_read_value)(void *const, void *, size_t);
  int (*fp_skip_value)(void *const, size_t);

  /* We need a start model to implement easy conversion to XML */
  int (*fp_get_encoding)(void *const, char *, size_t);
};

/* common reader vtable */
struct reader_vtable {
  struct object_prv_vtable const object;
  struct reader_prv_vtable const reader;
};

/* common reader object */
struct dicm_reader {
  struct reader_vtable const *vtable;
  struct dicm_io *src;
};

/* common reader interface */
#define dicm_reader_get_attribute(t, da) \
  ((t)->vtable->reader.fp_get_attribute((t), (da)))
#define dicm_reader_get_value_length(t, s) \
  ((t)->vtable->reader.fp_get_value_length((t), (s)))
#define dicm_reader_read_value(t, b, s) \
  ((t)->vtable->reader.fp_read_value((t), (b), (s)))
#define dicm_reader_skip_value(t, s) \
  ((t)->vtable->reader.fp_skip_value((t), (s)))
#define dicm_reader_get_encoding(t, e, s) \
  ((t)->vtable->reader.fp_get_encoding((t), (e), (s)))

/* return true only if there is a next event, false otherwise */
DICM_EXPORT bool dicm_reader_hasnext(const struct dicm_reader *);

/* return the next event */
DICM_EXPORT int dicm_reader_next_event(struct dicm_reader *);

DICM_EXPORT int dicm_reader_create(struct dicm_reader **pself,
                                   struct dicm_io *src, const char *encoding);
DICM_EXPORT int dicm_reader_utf8_create(struct dicm_reader **pself,
                                        struct dicm_io *src);
