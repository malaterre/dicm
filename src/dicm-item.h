#pragma once

#include "dicm-public.h"
#include "dicm-reader.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct dicm_item_reader {
  /* the current item state */
  enum state current_item_state;

  /* the current attribute */
  struct dicm_attribute da;

  /* current pos in value_length */
  uint32_t value_length_pos;
};

int dicm_item_reader_next(struct dicm_item_reader *self, struct dicm_io *src);

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

static inline struct dicm_item_reader *array_get(struct array *arr,
                                                 size_t index) {
  assert(index < arr->size);
  return &arr->data[index];
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
