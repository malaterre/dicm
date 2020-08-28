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

#include <stddef.h>

typedef long long offset_t;

struct _src {
  const struct _src_ops *ops;
  void *data;
};

struct _src_ops {
  /** Return 0 on success */
  int (*open)(struct _src *src, const char *fspec);
  /** Return 0 on success */
  int (*close)(struct _src *src);
  /** Return the actual size of buffer read */
  size_t (*read)(struct _src *src, void *buf, size_t bsize);
  /** Return 0 on success */
  int (*seek)(struct _src *src, offset_t offset);
  /** Return offset in file */
  offset_t (*tell)(struct _src *src);
};

/** dest */
struct _dst {
  const struct _dst_ops *ops;
  void *data;
};

struct _dst_ops {
  int (*open)(struct _dst *dst, const char *fspec);
  int (*close)(struct _dst *dst);
  /**
   * bsize in bytes
   */
  size_t (*write)(struct _dst *dst, void *buf, size_t bsize);
};
