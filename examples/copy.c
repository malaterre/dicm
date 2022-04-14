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
#include "writer.h"

#include "dicm-public.h"
#include <stdio.h>

static void copy_file_preamble(struct _writer *writer,
                               const struct _dicm_filepreamble *fp) {
  struct dicm_io *dst = writer->dst;
  // dst->ops->write(dst, fp->data, sizeof fp->data);
  int err = dicm_io_write(dst, fp->data, sizeof fp->data);
  assert(err == 0);
}

static void copy_prefix(struct _writer *writer,
                        const struct _dicm_prefix *prefix) {
  struct dicm_io *dst = writer->dst;
  // dst->ops->write(dst, prefix->data, sizeof prefix->data);
  int err = dicm_io_write(dst, prefix->data, sizeof prefix->data);
  assert(err == 0);
}

static void copy(struct _writer *writer, const struct _dataelement *de) {
  struct _dicm_sreader *sreader = writer->sreader;
  struct dicm_io *dst = writer->dst;
  struct _dataset *dataset = dicm_sreader_get_dataset(sreader);
  // dst->ops->write(dst, dataset->buffer, dataset->bufsize);
  int err = dicm_io_write(dst, dataset->buffer, dataset->bufsize);
  assert(err == 0);

  if (de->vl != kUndefinedLength) {
    char buf[4096];
    size_t len;
    while ((len = dicm_sreader_pull_dataelement_value(sreader, de, buf,
                                                      sizeof buf))) {
      // dst->ops->write(dst, buf, len);
      err = dicm_io_write(dst, buf, len);
      assert(err == 0);
    }
  }
}

static void copy_filemetaelement(struct _writer *writer,
                                 const struct _filemetaelement *fme) {
  copy(writer, (const struct _dataelement *)fme);
}

static void copy_item(struct _writer *writer, const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_bot(struct _writer *writer, const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_fragment(struct _writer *writer,
                          const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_end_item(struct _writer *writer,
                          const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_end_sq(struct _writer *writer, const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_end_frags(struct _writer *writer,
                           const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_sequenceofitems(struct _writer *writer,
                                 const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_sequenceoffragments(struct _writer *writer,
                                     const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_dataelement(struct _writer *writer,
                             const struct _dataelement *de) {
  copy(writer, de);
}

const struct _writer_ops copy_writer = {
    .write_file_preamble = copy_file_preamble,
    .write_prefix = copy_prefix,
    .write_filemetaelement = copy_filemetaelement,
    .write_dataelement = copy_dataelement,
    .write_sequenceofitems = copy_sequenceofitems,
    .write_sequenceoffragments = copy_sequenceoffragments,
    .write_item = copy_item,
    .write_bot = copy_bot,
    .write_fragment = copy_fragment,
    .write_end_item = copy_end_item,
    .write_end_sq = copy_end_sq,
    .write_end_frags = copy_end_frags,
};
