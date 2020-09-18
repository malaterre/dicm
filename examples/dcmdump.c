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

static unsigned int dcmdump_level = 0;

static void dcmdump_file_preamble(__maybe_unused const struct _dicm_filepreamble *fp) {
  /*
    const char *buf = fp->data;
    for (int i = 0; i < 128; ++i) {
      printf("%02x", (unsigned char)buf[i]);
    }
    printf("\n");
  */
  printf("\n");
  printf("# Dicom-File-Format\n");
  printf("\n");
  printf("# Dicom-Meta-Information-Header\n");
  printf("# Used TransferSyntax: Little Endian Explicit\n");
}

static void dcmdump_prefix(__maybe_unused const struct _dicm_prefix *prefix) {
//  const char *buf = prefix->data;
//  printf("%.4s\n", buf);
}

static void dcmdump_filemetaelement(const struct _filemetaelement *fme) {
  char buf[16];
  dicm_sreader_pull_dataelement_value(NULL, (const struct _dataelement *)fme,
                                      buf, sizeof buf);

  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf("(%04x,%04x) %.2s %d\n", (unsigned int)get_group(fme->tag),
         (unsigned int)get_element(fme->tag), get_vr(fme->vr), fme->vl);
}

static void dcmdump_item(__maybe_unused const struct _dataelement *de) {
  printf(">>\n");
}

static void dcmdump_bot(__maybe_unused const struct _dataelement *de) {
  printf(">>\n");
}

static void dcmdump_fragment(__maybe_unused const struct _dataelement *de) {
  printf(">>\n");
}

static void dcmdump_end_item(__maybe_unused const struct _dataelement *de) {}

static void dcmdump_end_sq(__maybe_unused const struct _dataelement *de) {
  assert(dcmdump_level > 0);
  --dcmdump_level;
}

static void dcmdump_end_frags(__maybe_unused const struct _dataelement *de) {
  assert(dcmdump_level > 0);
  --dcmdump_level;
}

static void dcmdump_sequenceofitems(const struct _dataelement *de) {
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
  ++dcmdump_level;
}

static void dcmdump_sequenceoffragments(const struct _dataelement *de) {
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
  ++dcmdump_level;
}

static void dcmdump_dataelement(const struct _dataelement *de) {
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
}

const struct _writer_ops dcmdump_writer = {
    .print_file_preamble = dcmdump_file_preamble,
    .print_prefix = dcmdump_prefix,
    .print_filemetaelement = dcmdump_filemetaelement,
    .print_dataelement = dcmdump_dataelement,
    .print_sequenceofitems = dcmdump_sequenceofitems,
    .print_sequenceoffragments = dcmdump_sequenceoffragments,
    .print_item = dcmdump_item,
    .print_bot = dcmdump_bot,
    .print_fragment = dcmdump_fragment,
    .print_end_item = dcmdump_end_item,
    .print_end_sq = dcmdump_end_sq,
    .print_end_frags = dcmdump_end_frags,
};
