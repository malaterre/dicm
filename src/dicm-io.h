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
#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef long long offset_t;

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
