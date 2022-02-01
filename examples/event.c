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

static unsigned int event_level = 0;

static void event_start_fmi(__maybe_unused struct _writer *writer) {
  printf("kStartFileMetaInformation\n");
}

static void event_end_fmi(__maybe_unused struct _writer *writer) {
  printf("kEndFileMetaInformation\n");
}

static void event_fmi_gl(__maybe_unused struct _writer *writer,
                         __maybe_unused uint32_t gl) {
  printf("kFileMetaInformationGroupLength\n");
}

static void event_group_gl(__maybe_unused struct _writer *writer,
                           __maybe_unused uint32_t gl) {
  printf("kGroupLengthDataElement\n");
}

static void event_end_group(__maybe_unused struct _writer *writer) {
  printf("kEndGroupDataElement\n");
}

static void event_file_preamble(
    __maybe_unused struct _writer *writer,
    __maybe_unused const struct _dicm_filepreamble *fp) {
  printf("kFilePreamble\n");
}

static void event_prefix(__maybe_unused struct _writer *writer,
                         __maybe_unused const struct _dicm_prefix *prefix) {
  printf("kPrefix\n");
}

static void event_filemetaelement(
    __maybe_unused struct _writer *writer,
    __maybe_unused const struct _filemetaelement *fme) {
  printf("kFileMetaElement\n");
}

static void event_item(__maybe_unused struct _writer *writer,
                       __maybe_unused const struct _dataelement *de) {
  if (event_level) printf("%*c", 2 * event_level, ' ');
  printf("kItem %u\n", de->vl);
}

static void event_bot(__maybe_unused struct _writer *writer,
                      __maybe_unused const struct _dataelement *de) {
  if (event_level) printf("%*c", 2 * event_level, ' ');
  printf("kBasicOffsetTable\n");
}

static void event_fragment(__maybe_unused struct _writer *writer,
                           __maybe_unused const struct _dataelement *de) {
  if (event_level) printf("%*c", 2 * event_level, ' ');
  printf("kFragment\n");
}

static void event_end_item(__maybe_unused struct _writer *writer,
                           __maybe_unused const struct _dataelement *de) {
  if (event_level) printf("%*c", 2 * event_level, ' ');
  printf("kItemDelimitationItem\n");
}

static void event_end_sq(__maybe_unused struct _writer *writer,
                         __maybe_unused const struct _dataelement *de) {
  assert(event_level > 0);
  --event_level;
  if (event_level) printf("%*c", 2 * event_level, ' ');
  printf("kSequenceOfItemsDelimitationItem\n");
}

static void event_end_frags(__maybe_unused struct _writer *writer,
                            __maybe_unused const struct _dataelement *de) {
  assert(event_level > 0);
  --event_level;
  if (event_level) printf("%*c", 2 * event_level, ' ');
  printf("kSequenceOfFragmentsDelimitationItem\n");
}

static void event_sequenceofitems(
    __maybe_unused struct _writer *writer,
    __maybe_unused const struct _dataelement *de) {
  if (event_level) printf("%*c", 2 * event_level, ' ');
  printf("kSequenceOfItems %u\n", de->vl);
  ++event_level;
}

static void event_sequenceoffragments(
    __maybe_unused struct _writer *writer,
    __maybe_unused const struct _dataelement *de) {
  if (event_level) printf("%*c", 2 * event_level, ' ');
  printf("kSequenceOfFragments\n");
  ++event_level;
}

static void event_dataelement(__maybe_unused struct _writer *writer,
                              __maybe_unused const struct _dataelement *de) {
  if (event_level) printf("%*c", 2 * event_level, ' ');
  printf("kDataElement\n");
}

const struct _writer_ops event_writer = {
    // FMI
    .write_start_fmi = event_start_fmi,
    .write_file_preamble = event_file_preamble,      // OPT
    .write_prefix = event_prefix,                    // OPT
    .write_fmi_gl = event_fmi_gl,                    // OPT
    .write_filemetaelement = event_filemetaelement,  // OPT
    .write_end_fmi = event_end_fmi,

    // DataSet:
    // DataElement
    .write_dataelement = event_dataelement,
    // Group
    .write_group_gl = event_group_gl,    // OPT
    .write_end_group = event_end_group,  // OPT
    // SQ
    .write_sequenceofitems = event_sequenceofitems,
    .write_end_sq = event_end_sq,  // (OPT)
    // Item
    .write_item = event_item,
    .write_end_item = event_end_item,  // (OPT)
    // SQ of Frags
    .write_sequenceoffragments = event_sequenceoffragments,
    .write_bot = event_bot,
    .write_fragment = event_fragment,
    .write_end_frags = event_end_frags,
};
