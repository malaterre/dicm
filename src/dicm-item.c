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

bool dicm_vr_is_16(dicm_vr_t vr) { return _is_vr16(vr); }

static int _item_reader_next_impl(struct dicm_item_reader *self,
                                  struct dicm_io *src) {
  union _ude ude;
  _Static_assert(16 == sizeof(ude), "16 bytes");
  _Static_assert(12 == sizeof(struct _ede32), "12 bytes");
  _Static_assert(8 == sizeof(struct _ede16), "8 bytes");
  _Static_assert(8 == sizeof(struct _ide), "8 bytes");
  int err = dicm_io_read(src, ude.bytes, 8);
  if (err) {
    return kEndModel;
  }

  const dicm_tag_t tag = _ide_get_tag(&ude);
  self->da.tag = tag;
  switch (tag) {
    case TAG_STARTITEM:
      self->da.vr = VR_NONE;
      self->da.vl = ude.ide.vl;
      return kStartItem;
    case TAG_ENDITEM:
      self->da.vr = VR_NONE;
      self->da.vl = ude.ide.vl;
      return kEndItem;
    case TAG_ENDSQITEM:
      self->da.vr = VR_NONE;
      self->da.vl = ude.ide.vl;
      return kEndSequence;
  }

  const dicm_vr_t vr = _ede16_get_vr(&ude);
  self->da.vr = vr;
  if (_is_vr16(vr)) {
    const dicm_vl_t vl = _ede16_get_vl(&ude);
    self->da.vl = vl;

    return kStartAttribute;
  }

  assert(ude.ede16.vl16 == 0);
  err = dicm_io_read(src, &ude.ede32.vl, 4);
  assert(err == 0);

  const dicm_vl_t vl = _ede32_get_vl(&ude);
  self->da.vl = vl;
  if (vr == VR_SQ || dicm_attribute_is_encapsulated_pixel_data(&self->da)) {
    return kStartSequence;
  }
  assert(!dicm_vl_is_undefined(self->da.vl));

  return kStartAttribute;
}

static int _item_reader_next_impl2(struct dicm_item_reader *self) {
  self->value_length_pos = 0;

  return kValue;
}

int dicm_item_reader_next(struct dicm_item_reader *self, struct dicm_io *src) {
  const enum state current_state = self->current_item_state;
  enum state next;
  if (current_state == kStartAttribute) {
    next = _item_reader_next_impl2(self);
  } else if (current_state == kValue) {
    // TODO: check user has consumed everything
    assert(self->da.vl == self->value_length_pos);
    next = _item_reader_next_impl(self, src);
  } else if (current_state == kStartSequence) {
    next = _item_reader_next_impl(self, src);
  } else if (current_state == kEndSequence) {
    next = _item_reader_next_impl(self, src);
  } else if (current_state == kStartItem) {
    next = _item_reader_next_impl(self, src);
  } else if (current_state == kEndItem) {
    next = _item_reader_next_impl(self, src);
  } else {
    assert(0);
  }
  self->current_item_state = next;
  return next;
}
