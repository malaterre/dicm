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

static void default_file_preamble(__maybe_unused struct _writer *writer,
                                  const struct _dicm_filepreamble *fp) {
  const char *buf = fp->data;
  for (int i = 0; i < 128; ++i) {
    printf("%02x", (unsigned char)buf[i]);
  }
  printf("\n");
}

static void default_prefix(__maybe_unused struct _writer *writer,
                           const struct _dicm_prefix *prefix) {
  const char *buf = prefix->data;
  printf("%.4s\n", buf);
}

static void default_filemetaelement(__maybe_unused struct _writer *writer,
                                    const struct _filemetaelement *fme) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(fme->tag),
         (unsigned int)get_element(fme->tag), get_vr(fme->vr), fme->vl);
}

static void default_item(__maybe_unused struct _writer *writer,
                         __maybe_unused const struct _dataelement *de) {
  printf(">>\n");
}

static void default_bot(__maybe_unused struct _writer *writer,
                        __maybe_unused const struct _dataelement *de) {
  printf(">>\n");
}

static void default_fragment(__maybe_unused struct _writer *writer,
                             __maybe_unused const struct _dataelement *de) {
  printf(">>\n");
}

static void default_end_item(__maybe_unused struct _writer *writer,
                             __maybe_unused const struct _dataelement *de) {}

static void default_end_sq(__maybe_unused struct _writer *writer,
                           __maybe_unused const struct _dataelement *de) {
  assert(default_level > 0);
  --default_level;
}

static void default_end_frags(__maybe_unused struct _writer *writer,
                              __maybe_unused const struct _dataelement *de) {
  assert(default_level > 0);
  --default_level;
}

static void default_sequenceofitems(__maybe_unused struct _writer *writer,
                                    const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
  ++default_level;
}

static void default_sequenceoffragments(__maybe_unused struct _writer *writer,
                                        const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
  ++default_level;
}

static void default_dataelement(__maybe_unused struct _writer *writer,
                                const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
}

const struct _writer_ops default_writer = {
    .write_file_preamble = default_file_preamble,
    .write_prefix = default_prefix,
    .write_filemetaelement = default_filemetaelement,
    .write_dataelement = default_dataelement,
    .write_sequenceofitems = default_sequenceofitems,
    .write_sequenceoffragments = default_sequenceoffragments,
    .write_item = default_item,
    .write_bot = default_bot,
    .write_fragment = default_fragment,
    .write_end_item = default_end_item,
    .write_end_sq = default_end_sq,
    .write_end_frags = default_end_frags,
};
