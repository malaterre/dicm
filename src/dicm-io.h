/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include <stdbool.h> /* bool */
#include <stddef.h>  /* size_t */
#include <stdint.h>  /* int64_t */

#include "dicm-features.h"

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

// FIXME: prefer enum ?
#define DICM_IO_READ 1
#define DICM_IO_WRITE 2

DICM_CHECK_RETURN int dicm_io_file_create(struct dicm_io **pself,
                                          const char *filename,
                                          int io_mode) DICM_NONNULL;

DICM_CHECK_RETURN int dicm_io_stream_create(struct dicm_io **pself,
                                            int io_mode) DICM_NONNULL;

#if 0
struct _src {
  const struct _src_ops *ops;
  void *data;
};

struct _src_ops {
  /** Return true on success */
  bool (*open)(struct _src *src, const char *fspec);
  /** Return true on success */
  bool (*close)(struct _src *src);
  /** Return the actual size of buffer read.
   * If an error occurs, or the end of the file is reached, the return value is
   * a short item count (or zero).
   */
  size_t (*read)(struct _src *src, void *buf, size_t bsize);
  /**
   * Return true if the source is at end
   */
  bool (*at_end)(struct _src *src);
  /** Seek to current position + offset. Return true on success */
  bool (*seek)(struct _src *src, offset_t offset);
  /** Return offset in file, or -1 upon error */
  offset_t (*tell)(struct _src *src);
};

/** dest */
struct _dst {
  const struct _dst_ops *ops;
  void *data;
};

struct _dst_ops {
  bool (*open)(struct _dst *dst, const char *fspec);
  bool (*close)(struct _dst *dst);
  /**
   * bsize in bytes
   */
  size_t (*write)(struct _dst *dst, const void *buf, size_t bsize);
};

typedef struct _src src_t;
typedef struct _dst dst_t;
#endif
