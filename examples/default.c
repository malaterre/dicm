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

static unsigned int default_level = 0;

static void default_file_preamble(struct _writer *writer,
const struct _dicm_filepreamble *fp) {
  const char *buf = fp->data;
  for (int i = 0; i < 128; ++i) {
    printf("%02x", (unsigned char)buf[i]);
  }
  printf("\n");
}

static void default_prefix(struct _writer *writer,
const struct _dicm_prefix *prefix) {
  const char *buf = prefix->data;
  printf("%.4s\n", buf);
}

static void default_filemetaelement(struct _writer *writer,
const struct _filemetaelement *fme) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(fme->tag),
         (unsigned int)get_element(fme->tag), get_vr(fme->vr), fme->vl);
}

static void default_item(struct _writer *writer,
__maybe_unused const struct _dataelement *de) {
  printf(">>\n");
}

static void default_bot(struct _writer *writer,
__maybe_unused const struct _dataelement *de) {
  printf(">>\n");
}

static void default_fragment(struct _writer *writer,
__maybe_unused const struct _dataelement *de) {
  printf(">>\n");
}

static void default_end_item(struct _writer *writer,
__maybe_unused const struct _dataelement *de) {}

static void default_end_sq(struct _writer *writer,
__maybe_unused const struct _dataelement *de) {
  assert(default_level > 0);
  --default_level;
}

static void default_end_frags(struct _writer *writer,
__maybe_unused const struct _dataelement *de) {
  assert(default_level > 0);
  --default_level;
}

static void default_sequenceofitems(struct _writer *writer,
const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
  ++default_level;
}

static void default_sequenceoffragments(struct _writer *writer,
const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
  ++default_level;
}

static void default_dataelement(struct _writer *writer,
const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
}

const struct _writer_ops default_writer = {
    .print_file_preamble = default_file_preamble,
    .print_prefix = default_prefix,
    .print_filemetaelement = default_filemetaelement,
    .print_dataelement = default_dataelement,
    .print_sequenceofitems = default_sequenceofitems,
    .print_sequenceoffragments = default_sequenceoffragments,
    .print_item = default_item,
    .print_bot = default_bot,
    .print_fragment = default_fragment,
    .print_end_item = default_end_item,
    .print_end_sq = default_end_sq,
    .print_end_frags = default_end_frags,
};
