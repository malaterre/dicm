#pragma once

#include "dicm-public.h"
#include "dicm-reader.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct dicm_item_reader {
  /* the current item state */
  enum dicm_state current_item_state;

  /* the current attribute */
  struct dicm_attribute da;

  /* current pos in value_length */
  uint32_t value_length_pos;

  union {
    /* Sequence of Fragments: frag number */
    int32_t frag_num;

    /* Sequence of Items: item number */
    uint32_t item_num;
  } index;

  DICM_CHECK_RETURN int (*fp_next_event)(struct dicm_item_reader *self,
                                         struct dicm_io *src);
};

int dicm_ds_reader_next_event(struct dicm_item_reader *self,
                              struct dicm_io *src);

int dicm_item_reader_next_event(struct dicm_item_reader *self,
                                struct dicm_io *src);

int dicm_fragments_reader_next_event(struct dicm_item_reader *self,
                                     struct dicm_io *src);

struct array {
  size_t size;
  size_t capacity;
  struct dicm_item_reader *data;
};

static inline struct array *array_create(struct array *arr, size_t size) {
  assert(arr);
  arr->size = size;
  arr->capacity = size;
  if (size) {
    arr->data = malloc(size * sizeof(struct dicm_item_reader));
  } else {
    arr->data = NULL;
  }
  return arr;
}

static inline void array_free(struct array *arr) { free(arr->data); }

static inline struct dicm_item_reader *array_at(struct array *arr,
                                                const size_t index) {
  if (index < arr->size) return &arr->data[index];
  return NULL;
}

static inline void array_push_back(struct array *arr,
                                   struct dicm_item_reader *item_reader) {
  arr->size++;
  if (arr->size >= arr->capacity) {
    arr->capacity = 2 * arr->size;
    arr->data =
        realloc(arr->data, arr->capacity * sizeof(struct dicm_item_reader));
    memcpy(&arr->data[arr->size - 1], item_reader,
           sizeof(struct dicm_item_reader));
  } else {
    memcpy(&arr->data[arr->size - 1], item_reader,
           sizeof(struct dicm_item_reader));
  }
}

static inline struct dicm_item_reader *array_back(struct array *arr) {
  struct dicm_item_reader *item_reader = &arr->data[arr->size - 1];
  return item_reader;
}

static inline void array_pop_back(struct array *arr) {
  assert(arr->size);
  arr->size--;
}
