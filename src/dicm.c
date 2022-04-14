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
#include "dicm-private.h"
#include "dicm-reader.h"

#include <assert.h>
#include <byteswap.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static inline bool is_tag_start(const tag_t tag) { return false; }
static inline bool is_tag_end_item(const tag_t tag) { return false; }
static inline bool is_tag_end_sq(const tag_t tag) { return false; }

struct _vl16 {
  uint16_t vl;
};
struct _vr32 {
  vr_t vr;
  uint16_t reserved;
};
struct _ede32 {
  tag_t tag;
  struct _vr32 vr32;
  vl_t vl;
};  // explicit data element. 12 bytes

struct _ede16 {
  tag_t tag;
  vr_t vr;
  struct _vl16 vl16;
};  // explicit data element, VR 16. 8 bytes

struct _ide {
  tag_t tag;
  vl_t vl;
};  // implicit data element. 8 bytes

typedef union _ude {
  uint32_t bytes[4];    // 16 bytes, 32bits aligned
  struct _ede32 ede32;  // explicit data element (12 bytes)
  struct _ede16 ede16;  // explicit data element (8 bytes)
  struct _ide ide;      // implicit data element (8 bytes)
} ude_t;

static const char dicm_utf8[] = "UTF-8";

struct _dicm_utf8_reader {
  struct dicm_reader reader;

  /* data */

  /* the current state */
  enum state current_state;

  /* local storage of key */
  ude_t ude;

  uint32_t value_length;     /* remaining of value length when state is VALUE */
  uint32_t value_length_pos; /* current pos in value_length */
};

static DICM_CHECK_RETURN int _dicm_utf8_reader_destroy(void *self_)
    DICM_NONNULL;

static DICM_CHECK_RETURN int _dicm_utf8_reader_get_attribute(
    void *const, struct dicm_attribute *) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_utf8_reader_get_value_length(
    void *const, size_t *) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_utf8_reader_read_value(void *const, void *,
                                                          size_t) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_utf8_reader_skip_value(void *const,
                                                          size_t) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_utf8_reader_get_fragment(
    void *const, int *frag_num) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_utf8_reader_get_item(
    void *const, int *item_num) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_utf8_reader_get_sequence(
    void *const, struct dicm_attribute *) DICM_NONNULL;
static DICM_CHECK_RETURN int _dicm_utf8_reader_get_encoding(
    void *const, char *, size_t) DICM_NONNULL;

static struct reader_vtable const g_vtable = {
    /* object interface */
    .object = {.fp_destroy = _dicm_utf8_reader_destroy},
    /* reader interface */
    .reader = {.fp_get_attribute = _dicm_utf8_reader_get_attribute,
               .fp_get_value_length = _dicm_utf8_reader_get_value_length,
               .fp_read_value = _dicm_utf8_reader_read_value,
               .fp_skip_value = _dicm_utf8_reader_skip_value,
               .fp_get_fragment = _dicm_utf8_reader_get_fragment,
               .fp_get_item = _dicm_utf8_reader_get_item,
               .fp_get_sequence = _dicm_utf8_reader_get_sequence,
               .fp_get_encoding = _dicm_utf8_reader_get_encoding}};

bool dicm_reader_hasnext(const struct dicm_reader *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  const int current_state = self->current_state;
  return current_state != kEndModel;
}

static inline bool is_vr16(const struct _vr32 vr) {
  uint16_t val;
  memcpy(&val, vr.vr, sizeof val);
  switch (val) {
    case VR_AE:
    case VR_AS:
    case VR_AT:
    case VR_CS:
    case VR_DA:
    case VR_DS:
    case VR_DT:
    case VR_FD:
    case VR_FL:
    case VR_IS:
    case VR_LO:
    case VR_LT:
    case VR_PN:
    case VR_SH:
    case VR_SL:
    case VR_SS:
    case VR_ST:
    case VR_TM:
    case VR_UI:
    case VR_UL:
    case VR_US:
      return true;
  }
  return false;
}

static int dicm_reader_next_impl(const struct dicm_reader *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  const int current_state = self->current_state;
  struct dicm_io *src = self->reader.src;

  ude_t ude;
  _Static_assert(16 == sizeof(ude), "16 bytes");
  memset(ude.bytes, 0, 16);  // FIXME
  int err = dicm_io_read(src, ude.bytes, 8);
  if (err) {
    return kEndModel;
  }

  if (is_tag_start(ude.ide.tag)) {
    return kStartAttribute;
  } else if (is_tag_end_item(ude.ide.tag)) {
    return kStartAttribute;
  } else if (is_tag_end_sq(ude.ide.tag)) {
    return kStartAttribute;
  }

  // VR16 ?
  if (is_vr16(ude.ede32.vr32)) {  // FIXME: improve coding style (vr16!=vr32)
    memcpy(&self->ude, &ude, sizeof ude);
    self->value_length = ude.ede16.vl16.vl;
    return kStartAttribute;
  }

  err = dicm_io_read(src, (char *)ude.bytes + 8, 4);  // FIXME
  assert(err == 0);

  memcpy(&self->ude, &ude, sizeof ude);
  self->value_length = ude.ede32.vl;

  return kStartAttribute;
}

static int dicm_reader_next_impl2(const struct dicm_reader *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  self->value_length_pos = 0;

  return kValue;
}

int dicm_reader_next(const struct dicm_reader *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  const int current_state = self->current_state;
  int next;
  if (current_state == -1) {
    next = kStartModel;
  } else if (current_state == kStartModel) {
    next = dicm_reader_next_impl(self_);
  } else if (current_state == kStartAttribute) {
    next = dicm_reader_next_impl2(self_);
  } else if (current_state == kEndAttribute) {
    next = dicm_reader_next_impl(self_);
  } else if (current_state == kValue) {
    const uint32_t value_length = self->value_length;
    const uint32_t value_length_pos = self->value_length_pos;
    if (value_length == value_length_pos) {
      next = kEndAttribute;
    } else {
      next = dicm_reader_next_impl2(self_);
    }
  } else {
    assert(0);
  }
  self->current_state = next;
  return next;
}

int dicm_reader_create(struct dicm_reader **pself, struct dicm_io *src,
                       const char *encoding) {
  if (strcmp(encoding, dicm_utf8)) {
    return 1;
  }
  /* utf-8 */
  return dicm_reader_utf8_create(pself, src);
}

int dicm_reader_utf8_create(struct dicm_reader **pself, struct dicm_io *src) {
  struct _dicm_utf8_reader *self =
      (struct _dicm_utf8_reader *)malloc(sizeof(*self));
  if (self) {
    *pself = &self->reader;
    self->reader.vtable = &g_vtable;
    self->reader.src = src;
    self->current_state = -1;
    return 0;
  }
  return 1;
}

int _dicm_utf8_reader_destroy(void *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  free(self);
  return 0;
}

int _dicm_utf8_reader_get_attribute(void *self_, struct dicm_attribute *da) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  union {
    uint32_t t;
    uint16_t a[2];
  } u;
  u.t = self->ude.ide.tag;
  da->tag = u.a[0] << 16u | u.a[1];
  da->vr = 0;  // trailing \0
  if (is_vr16(
          self->ude.ede32.vr32)) {  // FIXME: improve coding style (vr16!=vr32)
    memcpy(&da->vr, self->ude.ede16.vr, 2);
    da->vl = self->ude.ede16.vl16.vl;
  } else {
    memcpy(&da->vr, self->ude.ede32.vr32.vr, 2);
    da->vl = self->ude.ede32.vl;
  }
  return 0;
}

int _dicm_utf8_reader_get_value_length(void *self_, size_t *s) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  *s = self->value_length;
  return 0;
}

int _dicm_utf8_reader_read_value(void *self_, void *b, size_t s) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  const uint32_t value_length = self->value_length;
  const size_t max_length = s;
  const uint32_t to_read =
      max_length < (size_t)value_length ? (uint32_t)max_length : value_length;

  struct dicm_io *src = self->reader.src;
  int err = dicm_io_read(src, b, to_read);
  self->value_length_pos += to_read;
  assert(self->value_length_pos <= self->value_length);

  return 0;
}
int _dicm_utf8_reader_skip_value(void *self_, size_t s) { assert(0); }

int _dicm_utf8_reader_get_fragment(void *self_, int *frag_num) { assert(0); }

int _dicm_utf8_reader_get_item(void *self_, int *item_num) { assert(0); }

int _dicm_utf8_reader_get_sequence(void *self_, struct dicm_attribute *da) {
  assert(0);
}

int _dicm_utf8_reader_get_encoding(void *self_, char *c, size_t s) {
  assert(s >= sizeof dicm_utf8);
  memcpy(c, dicm_utf8, sizeof dicm_utf8);
  return 0;
}
