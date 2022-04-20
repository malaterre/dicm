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
  enum state current_state;

  /* Fragments: frag number */
  int frag_num;

  /* SQ: item number */
  int item_num;

  /* item readers */
#if 0
  struct dicm_item_reader item_reader;
#else
  struct array item_readers;
#endif
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

int dicm_reader_next(const struct dicm_reader *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  const enum state current_state = self->current_state;
  enum state next;
  if (current_state == -1) {
    next = kStartModel;
  } else if (current_state == kStartModel) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(self->item_readers.size == 1);
    item_reader->current_item_state = kStartItem;
    item_reader->da.vl = VL_UNDEFINED;
    next = dicm_item_reader_next(item_reader, self->reader.src);
  } else if (current_state == kStartSequence) {
    assert(self->item_num == 0);
    self->item_num = 1;  // item starts at 1
    struct dicm_item_reader dummy = {};
    array_push_back(&self->item_readers, &dummy);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    item_reader->current_item_state = kStartSequence;
    next = dicm_item_reader_next(item_reader, self->reader.src);
    assert(next == kStartItem);
  } else if (current_state == kEndSequence) {
    array_pop_back(&self->item_readers);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    next = dicm_item_reader_next(item_reader, self->reader.src);
  } else if (current_state == kStartFragments) {
    assert(self->frag_num == -1);
    self->frag_num = 0;  // frag 0 is bot
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == kStartSequence);
    next = dicm_item_reader_next(item_reader, self->reader.src);
    assert(next == kStartItem);
    // Convert it manually:
    next = kStartFragment;
  } else if (current_state == kEndFragments) {
    self->frag_num = -1;  // reset fragment number
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    next = dicm_item_reader_next(item_reader, self->reader.src);
  } else if (current_state == kStartFragment) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == kStartItem);
    // Reset the position in value:
    item_reader->value_length_pos = 0;
    next = kValue;
  } else if (current_state == kValue) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    const uint32_t value_length = item_reader->da.vl;
    const uint32_t value_length_pos = item_reader->value_length_pos;
    if (value_length == value_length_pos) {
      next = kEndAttribute;
    } else {
      next = dicm_item_reader_next(item_reader, self->reader.src);
    }
  } else {
    assert(current_state == kStartAttribute || current_state == kEndAttribute ||
           current_state == kStartItem || current_state == kEndItem);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    const enum state item_next =
        dicm_item_reader_next(item_reader, self->reader.src);
    const bool is_encapsulated_pixel_data =
        dicm_attribute_is_encapsulated_pixel_data(&item_reader->da);

    if (self->frag_num >= 0) {
      if (item_next == kStartItem) {
        next = kStartFragment;
      } else if (item_next == kEndSequence) {
        next = kEndFragments;
      } else
        assert(0);
    } else if (is_encapsulated_pixel_data) {
      assert(item_next == kStartSequence);
      next = kStartFragments;
    } else {
      next = item_next;
    }
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
    array_create(&self->item_readers, 1);  // TODO: is it a good default ?
    self->current_state = -1;
    self->item_num = 0;   // item number start at 1, 0 means invalid
    self->frag_num = -1;  // frag 0 is bot
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
  *frag_num = self->frag_num;
  return 0;
}

int _dicm_utf8_reader_get_item(void *self_, int *item_num) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  *item_num = self->item_num;
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
