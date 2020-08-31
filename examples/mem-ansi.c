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

#include "dicm-mem.h"

#include "dicm-private.h"

#include <stdlib.h> /* malloc/free */

ptr_t ansi_alloc(__maybe_unused struct _mem *mem, size_t size) {
  return malloc(size);
}

void ansi_free(__maybe_unused struct _mem *mem, ptr_t ptr) { free(ptr); }

static const struct _mem_ops ansi_ops = {
    .alloc = ansi_alloc,
    .free = ansi_free,
};

struct _mem ansi = {
    .ops = &ansi_ops,
    .data = NULL,
};
