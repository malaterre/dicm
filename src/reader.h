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
  int (*fp_get_value)(void *const, void *, size_t *);

  /* kFragment */
  int (*fp_get_fragment)(void *const, int *frag_num);

  /* kItem */
  int (*fp_get_item)(void *const,
                     int *item_num); /* FIXME: prefer dicm_item_t */

  /* kSequence: valid for SQ and Pixel Data,OB,u/l */
  int (*fp_get_sequence)(void *const, struct dicm_attribute *);

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
#define dicm_reader_get_value(t, b, s) \
  ((t)->vtable->reader.fp_get_value((t), (b), (s)))
#define dicm_reader_get_fragment(t, f) \
  ((t)->vtable->reader.fp_get_fragment((t), (f)))
#define dicm_reader_get_item(t, i) ((t)->vtable->reader.fp_get_item((t), (i)))
#define dicm_reader_get_sequence(t, sq) \
  ((t)->vtable->reader.fp_get_sequence((t), (sq)))
#define dicm_reader_get_encoding(t, e, s) \
  ((t)->vtable->reader.fp_get_encoding((t), (e), (s)))

DICM_EXPORT bool dicm_reader_hasnext(const struct dicm_reader *);
DICM_EXPORT int dicm_reader_next(const struct dicm_reader *);

DICM_EXPORT int dicm_reader_create(struct dicm_reader **pself,
                                   struct dicm_io *src, const char *encoding);
DICM_EXPORT int dicm_reader_utf8_create(struct dicm_reader **pself,
                                        struct dicm_io *src);
