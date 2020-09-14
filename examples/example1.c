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

#include "dicm.h"

#include "dicm-log.h"
#include "dicm-parser.h"
#include "dicm-private.h"

extern struct _log dlog;
extern const struct _src_ops fsrc_ops;
extern const struct _dst_ops fdst_ops;
extern struct _mem ansi;

#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include <stdio.h>  /* fopen */
#include <stdlib.h> /* EXIT_SUCCESS */

struct _writer {
  void (*print_file_preamble)(const char *buf);
  void (*print_prefix)(const char *buf);
  void (*print_filemetaelement)(const struct _filemetaelement *de);
  void (*print_dataelement)(const struct _dataelement *de);
  void (*print_sequenceoffragments)(const struct _dataelement *de);
  void (*print_item)();
  void (*print_end_item)();
  void (*print_end_sq)();
};

static unsigned int default_level = 0;

static void default_file_preamble(const char *buf) {
  for (int i = 0; i < 128; ++i) {
    printf("%02x", (unsigned char)buf[i]);
  }
  printf("\n");
}

static void default_prefix(const char *buf) { printf("%.4s\n", buf); }

static void default_filemetaelement(const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
}

static void default_item() {
  ++default_level;
  printf(">>\n");
}

static void default_end_item() {}

static void default_end_sq() {
  assert(default_level > 0);
  --default_level;
}

static void default_sequenceoffragments(const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
}

static void default_dataelement(const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
}

static const struct _writer default_writer = {
    .print_file_preamble = default_file_preamble,
    .print_prefix = default_prefix,
    .print_filemetaelement = default_filemetaelement,
    .print_dataelement = default_dataelement,
    .print_sequenceoffragments = default_sequenceoffragments,
    .print_item = default_item,
    .print_end_item = default_end_item,
    .print_end_sq = default_end_sq,
};

static unsigned int event_level = 0;

static void event_file_preamble(const char *buf) {
  printf("kFilePreamble\n");
}

static void event_prefix(const char *buf) { printf("kPrefix\n", buf); }

static void event_filemetaelement(const char *buf) {
  printf("kFileMetaElement\n", buf);
}

static void event_item() {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("kItem\n");
}

static void event_end_item() {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("kItemDelimitationItem\n");
}

static void event_end_sq() {
  assert(default_level > 0);
  --default_level;
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("kSequenceDelimitationItem\n");
}

static void event_sequenceoffragments(const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("kSequenceOfFragments\n");
  ++default_level;
}

static void event_dataelement(const struct _dataelement *de) {
  if (default_level) printf("%*c", 1 << default_level, ' ');
  printf("kDataElement\n");
}

static const struct _writer event_writer = {
    .print_file_preamble = event_file_preamble,
    .print_prefix = event_prefix,
    .print_filemetaelement = event_filemetaelement,
    .print_dataelement = event_dataelement,
    .print_sequenceoffragments = event_sequenceoffragments,
    .print_item = event_item,
    .print_end_item = event_end_item,
    .print_end_sq = event_end_sq,
};

void process_writer(const struct _writer *writer, dicm_sreader_t *sreader) {
  struct _dataelement *de;
  const char *buf;
  while (dicm_sreader_hasnext(sreader)) {
    int next = dicm_sreader_next(sreader);
    switch (next) {
      case kStartInstance:
        break;

      case kFilePreamble:
        if ((buf = dicm_sreader_get_file_preamble(sreader)))
          writer->print_file_preamble(buf);
        break;

      case kPrefix:
        if ((buf = dicm_sreader_get_prefix(sreader))) writer->print_prefix(buf);
        break;

      case kFileMetaElement:
        if ((de = dicm_sreader_get_dataelement(sreader)))
          writer->print_filemetaelement(de);
        break;

      case kDataElement:
        if ((de = dicm_sreader_get_dataelement(sreader)))
          writer->print_dataelement(de);
        break;

      case kSequenceOfFragments:
        if ((de = dicm_sreader_get_dataelement(sreader)))
          writer->print_sequenceoffragments(de);
        break;

      case kItem:
        writer->print_item();
        break;

      case kItemDelimitationItem:
        writer->print_end_item();
        break;

      case kSequenceDelimitationItem:
        writer->print_end_sq();
        break;

      case kEndInstance:
        break;

      default:
        printf("wotsit: %d\n", next);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) return EXIT_FAILURE;
  const char *filename = argv[1];
  set_global_logger(&dlog);

  src_t fsrc;
  dst_t fdst;
  dicm_sreader_t *sreader;

  fsrc.ops = &fsrc_ops;
  fdst.ops = &fdst_ops;

  fsrc.ops->open(&fsrc, filename);
  fdst.ops->open(&fdst, "output.dcm");

  sreader = dicm_sreader_init(&ansi, &fsrc);
  //process_writer(&default_writer, sreader);
  process_writer(&event_writer, sreader);
  dicm_sreader_fini(sreader);

  fsrc.ops->close(&fsrc);
  fdst.ops->close(&fdst);

  return EXIT_SUCCESS;
}
