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

static const char *separator = NULL;

static void print_separator() {
  if (separator) printf(separator);
}
static void json_start_fmi(__maybe_unused struct _writer *writer) {}

static void json_end_fmi(__maybe_unused struct _writer *writer) {
  printf("{\n");
}

static void json_fmi_gl(__maybe_unused struct _writer *writer,
                        __maybe_unused uint32_t gl) {}

static void json_group_gl(__maybe_unused struct _writer *writer,
                          __maybe_unused uint32_t gl) {}

static void json_end_group(__maybe_unused struct _writer *writer) {}

static void json_file_preamble(
    __maybe_unused struct _writer *writer,
    __maybe_unused const struct _dicm_filepreamble *fp) {}

static void json_prefix(__maybe_unused struct _writer *writer,
                        __maybe_unused const struct _dicm_prefix *prefix) {}

static void json_filemetaelement(
    __maybe_unused struct _writer *writer,
    __maybe_unused const struct _filemetaelement *fme) {}

static void json_item(__maybe_unused struct _writer *writer,
                      __maybe_unused const struct _dataelement *de) {
  print_separator();
  printf("{\n");
  separator = NULL;
}

static void json_bot(__maybe_unused struct _writer *writer,
                     __maybe_unused const struct _dataelement *de) {}

static void json_fragment(__maybe_unused struct _writer *writer,
                          __maybe_unused const struct _dataelement *de) {}

static void json_end_item(__maybe_unused struct _writer *writer,
                          __maybe_unused const struct _dataelement *de) {
  printf("}\n");
}

static void json_end_sq(__maybe_unused struct _writer *writer,
                        __maybe_unused const struct _dataelement *de) {
  printf("]\n");
  printf("}\n");
  separator = ",\n";
}

static void json_end_frags(__maybe_unused struct _writer *writer,
                           __maybe_unused const struct _dataelement *de) {
  printf("]\n");
}

static void json_sequenceofitems(__maybe_unused struct _writer *writer,
                                 __maybe_unused const struct _dataelement *de) {
  print_separator();
  printf("\"%04x%04x\": {\n \"vr\": \"%.2s\",\n \"Value\": [\n",
         (unsigned int)get_group(de->tag), (unsigned int)get_element(de->tag),
         get_vr(de->vr));
  separator = NULL;
}

static void json_sequenceoffragments(
    __maybe_unused struct _writer *writer,
    __maybe_unused const struct _dataelement *de) {
  //  printf("[\n");
  print_separator();
  printf("\"%04x%04x\": {\n \"vr\": \"%.2s\",\n \"Value\": [\n",
         (unsigned int)get_group(de->tag), (unsigned int)get_element(de->tag),
         get_vr(de->vr));
  separator = NULL;
}

static void json_dataelement(__maybe_unused struct _writer *writer,
                             __maybe_unused const struct _dataelement *de) {
  print_separator();
  const char value[] = "123";
  printf("\"%04x%04x\": {\n \"vr\": \"%.2s\",\n \"Value\": [%s]\n}",
         (unsigned int)get_group(de->tag), (unsigned int)get_element(de->tag),
         get_vr(de->vr), value);

  separator = ",\n";
}

const struct _writer_ops json_writer = {
    // FMI
    .write_start_fmi = json_start_fmi,
    .write_file_preamble = json_file_preamble,      // OPT
    .write_prefix = json_prefix,                    // OPT
    .write_fmi_gl = json_fmi_gl,                    // OPT
    .write_filemetaelement = json_filemetaelement,  // OPT
    .write_end_fmi = json_end_fmi,

    // DataSet:
    // DataElement
    .write_dataelement = json_dataelement,
    // Group
    .write_group_gl = json_group_gl,    // OPT
    .write_end_group = json_end_group,  // OPT
    // SQ
    .write_sequenceofitems = json_sequenceofitems,
    .write_end_sq = json_end_sq,  // (OPT)
    // Item
    .write_item = json_item,
    .write_end_item = json_end_item,  // (OPT)
    // SQ of Frags
    .write_sequenceoffragments = json_sequenceoffragments,
    .write_bot = json_bot,
    .write_fragment = json_fragment,
    .write_end_frags = json_end_frags,
};
