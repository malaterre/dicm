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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static const char dicm_utf8[] = "UTF-8";

struct _dicm_utf8_reader {
  struct dicm_reader reader;

  /* data */
  /* the current state */
  enum dicm_state current_state;

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
               .fp_get_encoding = _dicm_utf8_reader_get_encoding}};

bool dicm_reader_hasnext(const struct dicm_reader *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  const int current_state = self->current_state;
  assert(current_state != STATE_INVALID);
  return current_state != STATE_ENDDATASET;
}

static inline enum dicm_event token2event(const enum dicm_token dicm_next) {
  enum dicm_event next;
  switch (dicm_next) {
    case TOKEN_ATTRIBUTE:
      next = EVENT_ATTRIBUTE;
      break;
    case TOKEN_VALUE:
      next = EVENT_VALUE;
      break;
    case TOKEN_FRAGMENT:
      next = EVENT_FRAGMENT;
      break;
    case TOKEN_STARTSEQUENCE:
    case TOKEN_STARTFRAGMENTS:
      next = EVENT_START_SEQUENCE;
      break;
    case TOKEN_ENDSQITEM:
      next = EVENT_END_SEQUENCE;
      break;
    case TOKEN_STARTITEM:
      next = EVENT_START_ITEM;
      break;
    case TOKEN_ENDITEM:
      next = EVENT_END_ITEM;
      break;
    case TOKEN_EOF:
      next = EVENT_END_DATASET;
      break;
    default:
      assert(0);
  }
  return next;
}

static inline bool is_root_dataset(const struct _dicm_utf8_reader *self) {
  return self->item_readers.size == 1;
}

// get current item reader
static inline struct dicm_item_reader *get_item_reader(
    struct array *item_readers, const enum dicm_state current_state) {
  assert(array_back(item_readers)->current_item_state == current_state);
  return array_back(item_readers);
}

static inline void push_item_reader(struct array *item_readers,
                                    const enum dicm_state current_state) {
  assert(current_state == STATE_STARTSEQUENCE);
  assert(array_back(item_readers)->current_item_state == current_state);

  struct dicm_item_reader new_item = {
      .current_item_state = current_state,
      .fp_next_event = dicm_item_reader_next_event};
  array_push_back(item_readers, &new_item);
}

static inline void push_fragments_reader(struct array *item_readers,
                                         const enum dicm_state current_state) {
  assert(current_state == STATE_STARTFRAGMENTS);
  assert(array_back(item_readers)->current_item_state == current_state);

  struct dicm_item_reader new_item = {
      .current_item_state = current_state,
      .fp_next_event = dicm_fragments_reader_next_event};
  array_push_back(item_readers, &new_item);
}

static inline void pop_item_reader(struct array *item_readers,
                                   const enum dicm_state current_state) {
  assert(array_back(item_readers)->current_item_state == current_state);

  array_pop_back(item_readers);
  struct dicm_item_reader *item_reader = array_back(item_readers);
  assert(item_reader->current_item_state == STATE_STARTSEQUENCE ||
         item_reader->current_item_state == STATE_STARTFRAGMENTS);
  item_reader->current_item_state = current_state;  // re-initialize
}

int dicm_reader_next_event(struct dicm_reader *self_) {
  struct _dicm_utf8_reader *self = (struct _dicm_utf8_reader *)self_;
  const enum dicm_state current_state = self->current_state;
#if 1
  // special init case
  if (current_state == STATE_INIT) {
    self->current_state = STATE_STARTDATASET;
    return EVENT_START_DATASET;
  }
  // else get next dicm token:
  switch (current_state) {
    case STATE_STARTSEQUENCE:
      push_item_reader(&self->item_readers, current_state);
      break;
    case STATE_STARTFRAGMENTS:
      push_fragments_reader(&self->item_readers, current_state);
      break;
    case STATE_ENDSEQUENCE:
      pop_item_reader(&self->item_readers, current_state);
      break;
    default:;
  }
  struct dicm_item_reader *item_reader =
      get_item_reader(&self->item_readers, current_state);
  const enum dicm_token dicm_next =
      item_reader->fp_next_event(item_reader, self->reader.src);
  const enum dicm_event next = token2event(dicm_next);
  self->current_state = item_reader->current_item_state;
#else
  if (current_state == STATE_INIT) {
    next = EVENT_START_DATASET;
    self->current_state = STATE_STARTDATASET;
  } else if (current_state == STATE_STARTDATASET) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == current_state);
    assert(is_root_dataset(self));
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
    next = token2event(dicm_next);
    assert(dicm_next == TOKEN_ATTRIBUTE);
    self->current_state = STATE_ATTRIBUTE;
    assert(item_reader->current_item_state == self->current_state);
  } else if (current_state == STATE_ATTRIBUTE) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == current_state);
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
    next = token2event(dicm_next);
    assert(dicm_next == TOKEN_VALUE || dicm_next == TOKEN_STARTSEQUENCE ||
           dicm_next == TOKEN_STARTFRAGMENTS);
    self->current_state =
        dicm_next == TOKEN_VALUE
            ? STATE_VALUE
            : (dicm_next == TOKEN_STARTSEQUENCE ? STATE_STARTSEQUENCE
                                                : STATE_STARTFRAGMENTS);
    assert(item_reader->current_item_state == self->current_state);
  } else if (current_state == STATE_VALUE) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == current_state);
    const uint32_t value_length = item_reader->da.vl;
    const uint32_t value_length_pos = item_reader->value_length_pos;
    assert(value_length == value_length_pos);
    const bool is_root = is_root_dataset(self);
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
    next = token2event(dicm_next);
    assert(dicm_next == TOKEN_ATTRIBUTE ||
           (dicm_next == TOKEN_ENDITEM && !is_root) ||
           (dicm_next == TOKEN_STARTITEM && !is_root) ||
           (dicm_next == TOKEN_ENDSQITEM && !is_root) ||
           (dicm_next == TOKEN_EOF && is_root));
    self->current_state =
        dicm_next == TOKEN_ENDSQITEM
            ? STATE_ENDFRAGMENTS
            : (dicm_next == TOKEN_STARTITEM
                   ? STATE_FRAGMENT
                   : (dicm_next == TOKEN_ATTRIBUTE
                          ? STATE_ATTRIBUTE
                          : (dicm_next == TOKEN_ENDITEM
                                 ? STATE_ENDITEM
                                 : (is_root ? STATE_ENDDATASET
                                            : STATE_INVALID))));
    assert(item_reader->current_item_state == self->current_state);
  } else if (current_state == STATE_STARTSEQUENCE) {
    struct dicm_item_reader *item_reader0 = array_back(&self->item_readers);
    assert(item_reader0->current_item_state == current_state);
    struct dicm_item_reader dummy;
    array_push_back(&self->item_readers, &dummy);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    item_reader->current_item_state = STATE_STARTSEQUENCE;
    item_reader->index.item_num = 1;
    item_reader->fp_next_event = dicm_item_reader_next_event;
    assert(!is_root_dataset(self));
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
    next = token2event(dicm_next);
    assert(dicm_next == TOKEN_STARTITEM || dicm_next == TOKEN_ENDSQITEM);
    self->current_state =
        dicm_next == TOKEN_STARTITEM ? STATE_STARTITEM : STATE_ENDSEQUENCE;
    assert(item_reader->current_item_state == self->current_state);
  } else if (current_state == STATE_STARTITEM) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == current_state);
    assert(!is_root_dataset(self));
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
    next = token2event(dicm_next);
    assert(dicm_next == TOKEN_ATTRIBUTE || dicm_next == TOKEN_ENDITEM);
    self->current_state =
        dicm_next == TOKEN_ATTRIBUTE ? STATE_ATTRIBUTE : STATE_ENDITEM;
    assert(item_reader->current_item_state == self->current_state);
  } else if (current_state == STATE_ENDITEM) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == current_state);
    assert(!is_root_dataset(self));
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
    next = token2event(dicm_next);
    assert(dicm_next == TOKEN_STARTITEM || dicm_next == TOKEN_ENDSQITEM);
    self->current_state =
        dicm_next == TOKEN_STARTITEM ? STATE_STARTITEM : STATE_ENDSEQUENCE;
    assert(item_reader->current_item_state == self->current_state);
  } else if (current_state == STATE_ENDSEQUENCE) {
    struct dicm_item_reader *item_reader0 = array_back(&self->item_readers);
    assert(item_reader0->current_item_state == current_state);
    array_pop_back(&self->item_readers);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == STATE_STARTSEQUENCE);
    item_reader->current_item_state = STATE_VALUE;  // re-initialize
    const bool is_root = is_root_dataset(self);
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
    next = token2event(dicm_next);
    assert((is_root && dicm_next == TOKEN_EOF) ||
           dicm_next == TOKEN_ATTRIBUTE ||
           (!is_root && dicm_next == TOKEN_ENDITEM));
    self->current_state =
        dicm_next == TOKEN_EOF
            ? STATE_ENDDATASET
            : (dicm_next == TOKEN_ATTRIBUTE ? STATE_ATTRIBUTE : STATE_ENDITEM);
    assert(item_reader->current_item_state == self->current_state);
  } else if (current_state == STATE_STARTFRAGMENTS) {
    struct dicm_item_reader *item_reader0 = array_back(&self->item_readers);
    assert(item_reader0->current_item_state == current_state);
    struct dicm_item_reader dummy;
    array_push_back(&self->item_readers, &dummy);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    item_reader->current_item_state = STATE_STARTFRAGMENTS;
    item_reader->index.frag_num = 0;
    item_reader->fp_next_event = dicm_fragments_reader_next_event;
    assert(!is_root_dataset(self));
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
#if 0
    next = START_PIXELDATA;
#else
    next = token2event(dicm_next);
#endif
    assert(dicm_next == TOKEN_STARTITEM);
    self->current_state = STATE_FRAGMENT;
    assert(item_reader->current_item_state == self->current_state);
  } else if (current_state == STATE_ENDFRAGMENTS) {
    struct dicm_item_reader *item_reader0 = array_back(&self->item_readers);
    assert(item_reader0->current_item_state == current_state);
    array_pop_back(&self->item_readers);
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == STATE_STARTFRAGMENTS);
    const bool is_root = is_root_dataset(self);
    item_reader->current_item_state = STATE_VALUE;  // re-initialize
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
    next = token2event(dicm_next);
    assert(dicm_next == TOKEN_ATTRIBUTE ||
           (!is_root && dicm_next == TOKEN_ENDITEM) ||
           (is_root && dicm_next == TOKEN_EOF));
    self->current_state =
        dicm_next == TOKEN_ATTRIBUTE
            ? STATE_ATTRIBUTE
            : (dicm_next == TOKEN_ENDITEM ? STATE_ENDITEM : STATE_ENDDATASET);
    assert(item_reader->current_item_state == self->current_state);
  } else if (current_state == STATE_FRAGMENT) {
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    assert(item_reader->current_item_state == current_state);
    assert(!is_root_dataset(self));
    dicm_next = item_reader->fp_next_event(item_reader, self->reader.src);
    next = token2event(dicm_next);
    assert(dicm_next == TOKEN_VALUE);
    self->current_state = STATE_VALUE;
    assert(item_reader->current_item_state == self->current_state);
  } else {
    assert(0);
  }
#endif
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
    self->current_state = STATE_INIT;
    array_create(&self->item_readers, 1);  // TODO: is it a good default ?
    struct dicm_item_reader *item_reader = array_back(&self->item_readers);
    item_reader->current_item_state = STATE_STARTDATASET;
    item_reader->fp_next_event = dicm_ds_reader_next_event;

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
  io_ssize err = dicm_io_read(src, b, to_read);
  (void)err;
  item_reader->value_length_pos += to_read;
  assert(item_reader->value_length_pos <= item_reader->da.vl);

  return 0;
}
int _dicm_utf8_reader_skip_value(void *self_, size_t s) { assert(0); }

int _dicm_utf8_reader_get_encoding(void *self_, char *c, size_t s) {
  assert(s >= sizeof dicm_utf8);
  memcpy(c, dicm_utf8, sizeof dicm_utf8);
  return 0;
}
