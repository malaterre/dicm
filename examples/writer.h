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

#include "dicm-parser.h"
#include "dicm.h"

struct _writer {
  const struct _writer_ops *ops;
  struct _dicm_sreader *sreader;  // data
  struct _dst *dst;
};

struct _writer_ops {
  // FIXME Should not expose FMI to user API
  void (*write_start_fmi)(struct _writer *writer);
  void (*write_end_fmi)(struct _writer *writer);
  void (*write_fmi_gl)(struct _writer *writer, uint32_t gl);
  void (*write_file_preamble)(struct _writer *writer,
                              const struct _dicm_filepreamble *fp);
  void (*write_prefix)(struct _writer *writer,
                       const struct _dicm_prefix *prefix);
  void (*write_filemetaelement)(struct _writer *writer,
                                const struct _filemetaelement *de);
  // DataSet:
  void (*write_dataelement)(struct _writer *writer,
                            const struct _dataelement *de);
  void (*write_end_group)(struct _writer *writer);
  void (*write_group_gl)(struct _writer *writer, uint32_t gl);
  void (*write_sequenceofitems)(struct _writer *writer,
                                const struct _dataelement *de);
  void (*write_sequenceoffragments)(struct _writer *writer,
                                    const struct _dataelement *de);
  void (*write_item)(struct _writer *writer, const struct _dataelement *de);
  void (*write_bot)(struct _writer *writer, const struct _dataelement *de);
  void (*write_fragment)(struct _writer *writer, const struct _dataelement *de);
  void (*write_end_item)(struct _writer *writer, const struct _dataelement *de);
  void (*write_end_sq)(struct _writer *writer, const struct _dataelement *de);
  void (*write_end_frags)(struct _writer *writer,
                          const struct _dataelement *de);
};
