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
//#include <byteswap.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool dicm_vr_is_16(dicm_vr_t vr) { return _is_vr16(vr); }

static inline bool _tag_is_valid(const dicm_tag_t tag) {
  // The following cases have been handled by design:
  assert(tag != TAG_STARTITEM && tag != TAG_ENDITEM && tag != TAG_ENDSQITEM);
  return dicm_tag_get_group(tag) >= 0x0008;
}

static inline bool _tag_is_creator(const dicm_tag_t tag) {
  const uint_fast16_t group = dicm_tag_get_group(tag);
  const uint_fast16_t element = dicm_tag_get_element(tag);
  return group % 2 == 1 && element == 0x0;
}

static inline bool _vr_is_valid(const dicm_vr_t vr) {
  const char *str = dicm_vr_get_string(vr);
  return str[0] >= 'A' && str[0] <= 'Z' && str[1] >= 'A' && str[1] <= 'Z';
}

static inline bool _vl_is_valid(const dicm_vl_t vl) {
  return dicm_vl_is_undefined(vl) || vl % 2 == 0;
}

static inline bool _attribute_is_valid(const struct dicm_attribute *da) {
  // 1. check triplet separately:
  const bool valid =
      _tag_is_valid(da->tag) && _vr_is_valid(da->vr) && _vl_is_valid(da->vl);
  if (!valid) return false;
  // 2. handle finer cases here:
  if (dicm_vl_is_undefined(da->vl) && da->vr != VR_SQ) {
    if (!dicm_attribute_is_encapsulated_pixel_data(da)) {
      return false;
    }
  }
  if (_tag_is_creator(da->tag)) {
    if (da->vr != VR_LO) return false;
  }
  return true;
}

static enum dicm_event _item_reader_next_impl(struct dicm_item_reader *self,
                                              struct dicm_io *src) {
  union _ude ude;
  _Static_assert(16 == sizeof(ude), "16 bytes");
  _Static_assert(12 == sizeof(struct _ede32), "12 bytes");
  _Static_assert(8 == sizeof(struct _ede16), "8 bytes");
  _Static_assert(8 == sizeof(struct _ide), "8 bytes");
  io_ssize ssize = dicm_io_read(src, ude.bytes, 8);
  if (ssize != 8) {
    return ssize == 0 ? EVENT_EOF : EVENT_INVALID_DATA;
  }

  const dicm_tag_t tag = _ide_get_tag(&ude);
  self->da.tag = tag;
  switch (tag) {
    case TAG_STARTITEM:
      self->da.vr = VR_NONE;
      self->da.vl = ude.ide.vl;
      assert(_vl_is_valid(ude.ide.vl));
      return EVENT_STARTITEM;
    case TAG_ENDITEM:
      self->da.vr = VR_NONE;
      self->da.vl = ude.ide.vl;
      assert(ude.ide.vl == 0);
      return EVENT_ENDITEM;
    case TAG_ENDSQITEM:
      self->da.vr = VR_NONE;
      self->da.vl = ude.ide.vl;
      assert(ude.ide.vl == 0);
      return EVENT_ENDSEQUENCE;
  }

  const dicm_vr_t vr = _ede16_get_vr(&ude);
  self->da.vr = vr;
  if (_is_vr16(vr)) {
    const dicm_vl_t vl = _ede16_get_vl(&ude);
    self->da.vl = vl;
  } else {
    if (ude.ede16.vl16 != 0) return EVENT_INVALID_DATA;

    ssize = dicm_io_read(src, &ude.ede32.vl, 4);
    if (ssize != 4) return EVENT_INVALID_DATA;

    const dicm_vl_t vl = _ede32_get_vl(&ude);
    self->da.vl = vl;
  }

  if (!_attribute_is_valid(&self->da)) {
    assert(0);
    return EVENT_INVALID_DATA2;
  }

  return EVENT_ATTRIBUTE;
}

static inline enum dicm_event _item_reader_next_impl2(
    struct dicm_item_reader *self) {
  const dicm_vr_t vr = self->da.vr;
  if (dicm_attribute_is_encapsulated_pixel_data(&self->da)) {
    return EVENT_STARTFRAGMENTS;
  } else if (vr == VR_SQ) {
    return EVENT_STARTSEQUENCE;
  } else {
    self->value_length_pos = 0;

    return EVENT_VALUE;
  }
}

int dicm_item_reader_next_event(struct dicm_item_reader *self,
                                struct dicm_io *src) {
  const enum dicm_state current_state = self->current_item_state;
  enum dicm_event next;
  switch (current_state) {
    case STATE_STARTSEQUENCE:
      next = _item_reader_next_impl(self, src);
      assert(next == EVENT_STARTITEM);
      self->current_item_state =
          next == EVENT_STARTITEM ? STATE_STARTITEM : STATE_INVALID;
      break;
    case STATE_STARTITEM:
      next = _item_reader_next_impl(self, src);
      assert(next == EVENT_ATTRIBUTE);
      self->current_item_state =
          next == EVENT_ATTRIBUTE ? STATE_ATTRIBUTE : STATE_INVALID;
      break;
    case STATE_ATTRIBUTE:
      next = _item_reader_next_impl2(self);
      assert(next == EVENT_VALUE || next == EVENT_STARTSEQUENCE ||
             next == EVENT_STARTFRAGMENTS);
      self->current_item_state =
          next == EVENT_VALUE ? STATE_VALUE
                         : (next == EVENT_STARTSEQUENCE ? STATE_STARTSEQUENCE
                                                   : (next == EVENT_STARTFRAGMENTS
                                                          ? STATE_STARTFRAGMENTS
                                                          : STATE_INVALID));
      break;
    case STATE_VALUE:
      // TODO: check user has consumed everything
      assert(self->da.vl == self->value_length_pos);
      next = _item_reader_next_impl(self, src);
      if (next == EVENT_EOF) {
        self->current_item_state = STATE_DONE;
      } else {
        assert(next == EVENT_ATTRIBUTE || next == EVENT_ENDITEM);
        self->current_item_state =
            next == EVENT_ATTRIBUTE
                ? STATE_ATTRIBUTE
                : (next == EVENT_ENDITEM ? STATE_ENDITEM : STATE_INVALID);
      }
      break;
    case STATE_ENDITEM:
      next = _item_reader_next_impl(self, src);
      assert(next == EVENT_STARTITEM || next == EVENT_ENDSEQUENCE);
      self->current_item_state =
          next == EVENT_STARTITEM
              ? STATE_STARTITEM
              : (next == EVENT_ENDSEQUENCE ? STATE_ENDSEQUENCE : STATE_INVALID);
      break;
    case STATE_ENDSEQUENCE:
      next = _item_reader_next_impl(self, src);
      assert(next == EVENT_ATTRIBUTE);
      self->current_item_state =
          next == EVENT_ATTRIBUTE ? STATE_ATTRIBUTE : STATE_INVALID;
      break;
    default:
      assert(0);
  }
  return next;
}

static inline enum dicm_event _fragment_reader_next_impl2(
    struct dicm_item_reader *self) {
  const dicm_vr_t vr = self->da.vr;
  assert(vr == VR_NONE);
  self->value_length_pos = 0;

  return EVENT_VALUE;
}

static enum dicm_event _fragment_reader_next_impl(struct dicm_item_reader *self,
                                                  struct dicm_io *src) {
  const enum dicm_event next = _item_reader_next_impl(self, src);
  switch (next) {
    case EVENT_STARTITEM:
      return EVENT_FRAGMENT;
    case EVENT_ENDSEQUENCE:
      return EVENT_ENDFRAGMENTS;
  }
  assert(0);
  return EVENT_INVALID_DATA;
}

int dicm_fragment_reader_next_event(struct dicm_item_reader *self,
                                    struct dicm_io *src) {
  const enum dicm_state current_state = self->current_item_state;
  enum dicm_event next;
  switch (current_state) {
    case STATE_STARTFRAGMENTS:
      next = _fragment_reader_next_impl(self, src);
      assert(next == EVENT_FRAGMENT);
      self->current_item_state =
          next == EVENT_FRAGMENT ? STATE_FRAGMENT : STATE_INVALID;
      break;
    case STATE_FRAGMENT:
      next = _fragment_reader_next_impl2(self);
      assert(next == EVENT_VALUE);
      self->current_item_state = next == EVENT_VALUE ? STATE_VALUE : STATE_INVALID;
      break;
    case STATE_VALUE:
      // TODO: check user has consumed everything
      assert(self->da.vl == self->value_length_pos);
      next = _fragment_reader_next_impl(self, src);
      assert(next == EVENT_FRAGMENT || next == EVENT_ENDFRAGMENTS);
      self->current_item_state =
          next == EVENT_FRAGMENT
              ? STATE_FRAGMENT
              : (next == EVENT_ENDFRAGMENTS ? STATE_ENDFRAGMENTS : STATE_INVALID);
      break;
    case STATE_ENDFRAGMENTS:
      // end of fragments read, go back to normal item reading:
      next = _item_reader_next_impl(self, src);
      if (next == EVENT_EOF) {
        self->current_item_state = STATE_DONE;
      } else {
        assert(next == EVENT_ATTRIBUTE);
        self->current_item_state =
            next == EVENT_ATTRIBUTE ? STATE_ATTRIBUTE : STATE_INVALID;
      }
      break;
    default:
      assert(0);
  }
  return next;
}
