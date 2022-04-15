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
#include "dicm-item.h"
#include "dicm-private.h"
#include "dicm-reader.h"

#include <assert.h>
#include <byteswap.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAKE_TAG(group, element) (group << 16u | element)

enum {
  kPixelDataTag = MAKE_TAG(0x7fe0, 0x0010),
  kStartItemTag = MAKE_TAG(0xfffe, 0xe000),
  kEndItemTag = MAKE_TAG(0xfffe, 0xe00d),
  kEndSQItemTag = MAKE_TAG(0xfffe, 0xe0dd),
};

static inline bool is_tag_start(const tag_t tag) {
  return tag == kStartItemTag;
}
static inline bool is_tag_end_item(const tag_t tag) {
  return tag == kEndItemTag;
}
static inline bool is_tag_end_sq(const tag_t tag) {
  return tag == kEndSQItemTag;
}

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

#if 0
bool dicm_item_reader_hasnext(const struct dicm_item_reader *self) {
  const int current_state = self->current_state;
  return current_state != kEndModel;
}
#endif

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

static void ude2attribute(ude_t *ude, struct dicm_attribute *da) {
#if 0
  union {
    uint32_t t;
    uint16_t a[2];
  } u;
  u.t = ude->ide.tag;
  da->tag = u.a[0] << 16u | u.a[1];
#else
  da->tag = ude->ide.tag;  // already byte-swapped
#endif
  da->vr = 0;                      // trailing \0
  if (is_vr16(ude->ede32.vr32)) {  // FIXME: improve coding style (vr16!=vr32)
    memcpy(&da->vr, ude->ede16.vr, 2);
    da->vl = ude->ede16.vl16.vl;
  } else {
    memcpy(&da->vr, ude->ede32.vr32.vr, 2);
    da->vl = ude->ede32.vl;
  }
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAP_TAG(t) t = (t >> 16) | (t >> 16)
#else
#error
#endif

int dicm_item_reader_next_impl(struct dicm_item_reader *self,
                               struct dicm_io *src) {
  ude_t ude;
  _Static_assert(16 == sizeof(ude), "16 bytes");
  memset(ude.bytes, 0, 16);  // FIXME
  int err = dicm_io_read(src, ude.bytes, 8);
  if (err) {
    return kEndModel;
  }
  // byte-swap tag:
  {
    union {
      uint32_t t;
      uint16_t a[2];
    } u;
    u.t = ude.ide.tag;
    ude.ide.tag = u.a[0] << 16u | u.a[1];
  }

  if (is_tag_start(ude.ide.tag)) {
    return kStartItem;
  } else if (is_tag_end_item(ude.ide.tag)) {
    return kEndItem;
  } else if (is_tag_end_sq(ude.ide.tag)) {
    return kEndSequence;
  }

  // VR16 ?
  if (is_vr16(ude.ede32.vr32)) {  // FIXME: improve coding style (vr16!=vr32)
#if 0
    memcpy(&self->ude, &ude, sizeof ude);
    self->value_length = ude.ede16.vl16.vl;
#else
    ude2attribute(&ude, &self->da);
#endif
    return kStartAttribute;
  }

  err = dicm_io_read(src, (char *)ude.bytes + 8, 4);  // FIXME
  assert(err == 0);

#if 0
  memcpy(&self->ude, &ude, sizeof ude);
  self->value_length = ude.ede32.vl;
#else
  ude2attribute(&ude, &self->da);
#endif
  if (self->da.vr == VR_SQ) {
    return kStartSequence;
  }

  return kStartAttribute;
}

int dicm_item_reader_next_impl2(struct dicm_item_reader *self) {
  self->value_length_pos = 0;

  return kValue;
}

int dicm_item_reader_next(struct dicm_item_reader *self, struct dicm_io *src) {
  const enum state current_state = self->current_item_state;
  enum state next;
  if (current_state == kStartAttribute) {
    next = dicm_item_reader_next_impl2(self);
  } else if (current_state == kEndAttribute) {
    next = dicm_item_reader_next_impl(self, src);
  } else if (current_state == kValue) {
#if 0
    const uint32_t value_length = self->value_length;
#else
    const uint32_t value_length = self->da.vl;
#endif
    const uint32_t value_length_pos = self->value_length_pos;
    if (value_length == value_length_pos) {
      next = kEndAttribute;
    } else {
      next = dicm_item_reader_next_impl2(self);
    }
  } else if (current_state == kStartSequence) {
    next = dicm_item_reader_next_impl(self, src);
  } else if (current_state == kStartItem) {
    next = dicm_item_reader_next_impl(self, src);
  } else if (current_state == kEndItem) {
    next = dicm_item_reader_next_impl(self, src);
  } else {
    assert(0);
  }
  self->current_item_state = next;
  return next;
}
