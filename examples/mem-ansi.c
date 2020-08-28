#include "dicm-mem.h"

#include <stdlib.h> /* malloc/free */

ptr_t ansi_alloc(struct _mem *mem, size_t size)
{
  return malloc(size);
}

void ansi_free(struct _mem *mem, ptr_t ptr)
{
  free(ptr);
}

static const struct _mem_ops ansi_ops = {
  .alloc = ansi_alloc,
  .free = ansi_free,
};
