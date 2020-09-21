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

#include "dicm-private.h"
#include <stdio.h>

static void copy_file_preamble(struct _writer *writer,
                               const struct _dicm_filepreamble *fp) {
  struct _dst *dst = writer->dst;
  dst->ops->write(dst, fp->data, sizeof fp->data);
}

static void copy_prefix(struct _writer *writer,
                        const struct _dicm_prefix *prefix) {
  struct _dst *dst = writer->dst;
  dst->ops->write(dst, prefix->data, sizeof prefix->data);
}

static void copy(struct _writer *writer, const struct _dataelement *de) {
  struct _dicm_sreader *sreader = writer->sreader;
  struct _dst *dst = writer->dst;
  struct _dataset *dataset = dicm_sreader_get_dataset(sreader);
  dst->ops->write(dst, dataset->buffer, dataset->bufsize);

  if (de->vl != kUndefinedLength) {
    char buf[4096];
    size_t len;
    while ((len = dicm_sreader_pull_dataelement_value(sreader, de, buf,
                                                      sizeof buf))) {
      dst->ops->write(dst, buf, len);
    }
  }
}

static void copy_filemetaelement(struct _writer *writer,
                                 const struct _filemetaelement *fme) {
  copy(writer, (const struct _dataelement *)fme);
}

static void copy_item(struct _writer *writer,
                      __maybe_unused const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_bot(struct _writer *writer,
                     __maybe_unused const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_fragment(struct _writer *writer,
                          __maybe_unused const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_end_item(struct _writer *writer,
                          __maybe_unused const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_end_sq(struct _writer *writer,
                        __maybe_unused const struct _dataelement *de) {
  copy(writer, de);
}

static void copy_end_frags(struct _writer *writer,
                           __maybe_unused const struct _dataelement *de) {
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
    .print_file_preamble = copy_file_preamble,
    .print_prefix = copy_prefix,
    .print_filemetaelement = copy_filemetaelement,
    .print_dataelement = copy_dataelement,
    .print_sequenceofitems = copy_sequenceofitems,
    .print_sequenceoffragments = copy_sequenceoffragments,
    .print_item = copy_item,
    .print_bot = copy_bot,
    .print_fragment = copy_fragment,
    .print_end_item = copy_end_item,
    .print_end_sq = copy_end_sq,
    .print_end_frags = copy_end_frags,
};
