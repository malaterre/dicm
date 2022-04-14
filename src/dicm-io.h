/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-public.h"

#include <stdbool.h> /* bool */
#include <stddef.h>  /* size_t */
#include <stdint.h>  /* int64_t */

/* ftello is signed 64 bits so mimic behavior here: */
typedef int64_t io_offset;

struct io_prv_vtable {
  DICM_CHECK_RETURN int (*fp_read)(void *const, void *, size_t) DICM_NONNULL;
  DICM_CHECK_RETURN int (*fp_seek)(void *const, io_offset) DICM_NONNULL;
  DICM_CHECK_RETURN int (*fp_tell)(void *const, io_offset *) DICM_NONNULL;
  DICM_CHECK_RETURN int (*fp_write)(void *const, void const *,
                                    size_t) DICM_NONNULL;
};

/* common io vtable */
struct io_vtable {
  struct object_prv_vtable const object;
  struct io_prv_vtable const io;
};

/* common io object */
struct dicm_io {
  struct io_vtable const *vtable;
};

/* common io interface */
#define dicm_io_read(t, b, s) ((t)->vtable->io.fp_read((t), (b), (s)))
#define dicm_io_write(t, b, s) ((t)->vtable->io.fp_write((t), (b), (s)))

enum IO_TYPES { DICM_IO_READ = 1, DICM_IO_WRITE = 2 };

DICM_CHECK_RETURN int dicm_io_file_create(struct dicm_io **pself,
                                          const char *filename,
                                          int io_mode) DICM_NONNULL;

DICM_CHECK_RETURN int dicm_io_stream_create(struct dicm_io **pself,
                                            int io_mode) DICM_NONNULL;
