#pragma once

#include <stddef.h> /* size_t */

struct _mem {
  const struct _mem_ops *ops;
  void *data;
};

typedef void* ptr_t;
struct _mem_ops {
  ptr_t (*alloc)(struct _mem *mem, size_t size);
  void (*free)(struct _mem *mem, ptr_t ptr);
};

