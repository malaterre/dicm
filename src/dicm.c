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
#include "dicm-public.h"
#include "dicm-reader.h"

#include <assert.h>
#include <byteswap.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static const char dicm_utf8[] = "UTF-8";

struct _dicm_utf8_reader {
  struct dicm_reader reader;

  /* data */
  /* the current state */
  enum ml_state current_state;

  /* item readers */
  struct array item_readers;
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

static inline enum ml_state dicm2ml(enum dicm_event dicm_next) {
  enum ml_state next;
  switch (dicm_next) {
    case kAttribute:
      next = kStartAttribute;
      break;
    case kValue:
      next = kBytes;
      break;
    case kFragment:
      next = kStartFragment;
      break;
    case kStartSequence:
      next = kStartArray;
      break;
    case kEndSequence:
      next = kEndArray;
      break;
    case kStartFragments:
      next = kStartPixelData;
      break;
    case kEndFragments:
      next = kEndPixelData;
      break;
    case kStartItem:
      next = kStartObject;
      break;
    case kEndItem:
      next = kEndObject;
      break;
    case kEOF:
    case -1:
      next = kEndModel;
      break;
    default:
      assert(0);
  }
  return next;
}

int dicm_reader_next_event(const struct dicm_reader *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  const enum ml_state current_state = self->current_state;
  enum ml_state next;
  enum dicm_event dicm_next;
  if (current_state == -1) {
    next = kStartModel;
  } else if (current_state == kStartModel) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(self->item_readers.size == 1);
    item_reader->current_item_state = STATE_STARTITEM;
    item_reader->da.vl = VL_UNDEFINED;
    dicm_next = dicm_item_reader_next_event(item_reader, self->reader.src);
    next = dicm2ml(dicm_next);
  } else if (current_state == kStartArray) {
    struct dicm_item_reader dummy = {};
    array_push_back(&self->item_readers, &dummy);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    item_reader->current_item_state = STATE_STARTSEQUENCE;
    item_reader->index.item_num = 1;
    dicm_next = dicm_item_reader_next_event(item_reader, self->reader.src);
    next = dicm2ml(dicm_next);
    assert(next == kStartObject);
  } else if (current_state == kEndArray) {
    array_pop_back(&self->item_readers);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    item_reader->current_item_state = STATE_ENDSEQUENCE;
    dicm_next = dicm_item_reader_next_event(item_reader, self->reader.src);
    next = dicm2ml(dicm_next);
  } else if (current_state == kStartPixelData) {
    struct dicm_item_reader dummy = {};
    array_push_back(&self->item_readers, &dummy);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    item_reader->current_item_state = STATE_STARTFRAGMENTS;
    item_reader->index.frag_num = 0;
    dicm_next = dicm_fragment_reader_next_event(item_reader, self->reader.src);
    next = dicm2ml(dicm_next);
    assert(next == kStartFragment);
  } else if (current_state == kEndPixelData) {
    array_pop_back(&self->item_readers);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    item_reader->current_item_state = STATE_ENDFRAGMENTS;
    dicm_next = dicm_fragment_reader_next_event(item_reader, self->reader.src);
    next = dicm2ml(dicm_next);
  } else if (current_state == kStartFragment) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == STATE_FRAGMENT);
    dicm_next = dicm_fragment_reader_next_event(item_reader, self->reader.src);
    assert(dicm_next == kValue);
    next = dicm2ml(dicm_next);
  } else if (current_state == kEndFragment) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == STATE_VALUE);
    dicm_next = dicm_fragment_reader_next_event(item_reader, self->reader.src);
    assert(dicm_next == kFragment || dicm_next == kEndFragments);
    next = dicm2ml(dicm_next);
  } else if (current_state == kBytes) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    const bool is_fragment = item_reader->da.tag == TAG_STARTITEM;
    const uint32_t value_length = item_reader->da.vl;
    const uint32_t value_length_pos = item_reader->value_length_pos;
    if (value_length == value_length_pos) {
      next = is_fragment ? kEndFragment : kEndAttribute;
    } else {
      assert(0);
      assert(is_fragment == false);
      dicm_next = dicm_item_reader_next_event(item_reader, self->reader.src);
      next = dicm2ml(dicm_next);
    }
  } else {
    assert(current_state == kStartAttribute || current_state == kEndAttribute ||
           current_state == kStartObject || current_state == kEndObject);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    dicm_next = dicm_item_reader_next_event(item_reader, self->reader.src);
    next = dicm2ml(dicm_next);
  }
  assert(next < 100);
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
    array_create(&self->item_readers, 1);  // TODO: is it a good default ?
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    item_reader->index.item_num = 0;  // item number start at 1, 0 means invalid
    item_reader->index.frag_num = -1;  // frag 0 is bot

    return 0;
  }
  return 1;
}

int _dicm_utf8_reader_destroy(void *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  array_free(&self->item_readers);
  free(self);
  return 0;
}

int _dicm_utf8_reader_get_attribute(void *self_, struct dicm_attribute *da) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  struct dicm_item_reader *item_reader = array_back(&self->item_readers);
  memcpy(da, &item_reader->da, sizeof *da);
  return 0;
}

int _dicm_utf8_reader_get_value_length(void *self_, size_t *s) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  struct dicm_item_reader *item_reader = array_back(&self->item_readers);
  *s = item_reader->da.vl;
  return 0;
}

int _dicm_utf8_reader_read_value(void *self_, void *b, size_t s) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  struct dicm_item_reader *item_reader = array_back(&self->item_readers);
  const uint32_t value_length = item_reader->da.vl;
  const size_t max_length = s;
  const uint32_t to_read =
      max_length < (size_t)value_length ? (uint32_t)max_length : value_length;

  struct dicm_io *src = self->reader.src;
  int err = dicm_io_read(src, b, to_read);
  item_reader->value_length_pos += to_read;
  assert(item_reader->value_length_pos <= item_reader->da.vl);

  return 0;
}
int _dicm_utf8_reader_skip_value(void *self_, size_t s) { assert(0); }

int _dicm_utf8_reader_get_fragment(void *self_, int *frag_num) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  struct dicm_item_reader *item_reader = array_back(&self->item_readers);
  *frag_num = item_reader->index.frag_num;
  return 0;
}

int _dicm_utf8_reader_get_item(void *self_, int *item_num) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  struct dicm_item_reader *item_reader = array_back(&self->item_readers);
  *item_num = item_reader->index.item_num;
  return 0;
}

int _dicm_utf8_reader_get_sequence(void *self_, struct dicm_attribute *da) {
  assert(0);
}

int _dicm_utf8_reader_get_encoding(void *self_, char *c, size_t s) {
  assert(s >= sizeof dicm_utf8);
  memcpy(c, dicm_utf8, sizeof dicm_utf8);
  return 0;
}
