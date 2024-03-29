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
#include <string.h>

static unsigned int dcmdump_level = 0;

static unsigned int first_dataelement = 0;

#ifdef FIXME
static void write_with_indent(int indent, const char *string) {
  printf("%*s%s", indent, "", string);
}
#endif

static void dcmdump_file_preamble(
    DICM_UNUSED struct _writer *writer,
    DICM_UNUSED const struct _dicm_filepreamble *fp) {
  printf("\n");
  printf("# Dicom-File-Format\n");
  printf("\n");
  printf("# Dicom-Meta-Information-Header\n");
  printf("# Used TransferSyntax: Little Endian Explicit\n");
}

static void dcmdump_prefix(DICM_UNUSED struct _writer *writer,
                           DICM_UNUSED const struct _dicm_prefix *prefix) {}

static void write_ob(char *str, const char *buf, size_t len) {
  char *pstr = str;
  for (unsigned int i = 0; i < len; ++i) {
    if (i != 0) {
      *pstr = '\\';
      ++pstr;
    }
    sprintf(pstr, "%02x", (unsigned char)buf[i]);
    pstr += 2;
  }
}

static void write_ow(char *str, const char *buf, size_t len) {
  char *pstr = str;
  for (unsigned int i = 0; i < len; i += 2) {
    if (i != 0) {
      *pstr = '\\';
      ++pstr;
    }
    sprintf(pstr, "%02x", (unsigned char)buf[i + 1]);
    pstr += 2;
    sprintf(pstr, "%02x", (unsigned char)buf[i]);
    pstr += 2;
  }
}

static void write_dataelement(struct _writer *writer,
                              const struct _dataelement *de) {
  char str[512];
  char buf[64];
  size_t len =
      dicm_sreader_pull_dataelement_value(writer->sreader, de, buf, sizeof buf);

  const unsigned int width = 40;
  if (de->vr == kUI || de->vr == kSH || de->vr == kAE || de->vr == kCS ||
      de->vr == kDA || de->vr == kTM || de->vr == kLO || de->vr == kPN ||
      de->vr == kDS || de->vr == kAS || de->vr == kIS || de->vr == kDT) {
    assert(len <= sizeof buf);
    unsigned int ilen = strnlen(buf, len /*sizeof buf*/);
    if (len < sizeof buf) buf[len] = 0;  // FIXME
    if (de->vl) {
      /*unsigned slen =*/snprintf(str, sizeof str, "[%*s]", ilen, buf);
    } else {
      sprintf(str, "(%s)", "no value available");
    }
  } else if (de->vr == kOB || de->vr == kOW) {
    if (de->vr == kOB)
      write_ob(str, buf, len);
    else if (de->vr == kOW)
      write_ow(str, buf, len);
    str[65] = '.';
    str[66] = '.';
    str[67] = '.';
    str[68] = 0;
  } else {
    memset(str, ' ', sizeof str);  // FIXME
    str[len + 1] = 0;
  }
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf("(%04x,%04x) %.2s %-*s # %*d,\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), width, str, 3,
         de->vl);
}

static void dcmdump_filemetaelement(struct _writer *writer,
                                    const struct _filemetaelement *fme) {
  write_dataelement(writer, (const struct _dataelement *)fme);
}

static void dcmdump_item(DICM_UNUSED struct _writer *writer,
                         DICM_UNUSED const struct _dataelement *de) {
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf(
      "(fffe,e000) na (Item with undefined length #=)        # u/l, 1 Item\n");
  ++dcmdump_level;
}

static void dcmdump_bot(DICM_UNUSED struct _writer *writer,
                        DICM_UNUSED const struct _dataelement *de) {
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf(
      "(fffe,e000) pi (no value available)                     #   0, 1 "
      "Item\n");
}

static void dcmdump_fragment(DICM_UNUSED struct _writer *writer,
                             DICM_UNUSED const struct _dataelement *de) {
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf(
      "(fffe,e000) pi (no value available)                     #   0, 1 "
      "Item\n");
}

static void dcmdump_end_item(DICM_UNUSED struct _writer *writer,
                             DICM_UNUSED const struct _dataelement *de) {
  assert(dcmdump_level > 0);
  --dcmdump_level;
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf(
      "(fffe,e00d) na (ItemDelimitationItem)                   #   0, 0 "
      "ItemDelimitationItem\n");
}

static void dcmdump_end_sq(DICM_UNUSED struct _writer *writer,
                           DICM_UNUSED const struct _dataelement *de) {
  assert(dcmdump_level > 0);
  --dcmdump_level;
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf(
      "(fffe,e0dd) na (SequenceDelimitationItem)               #   0, 0 "
      "SequenceDelimitationItem\n");
}

static void dcmdump_end_frags(DICM_UNUSED struct _writer *writer,
                              DICM_UNUSED const struct _dataelement *de) {
  assert(dcmdump_level > 0);
  --dcmdump_level;
  printf(
      "(fffe,e0dd) na (SequenceDelimitationItem)               #   0, 0 "
      "SequenceDelimitationItem\n");
}

static void dcmdump_sequenceofitems(DICM_UNUSED struct _writer *writer,
                                    const struct _dataelement *de) {
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf("(%04x,%04x) %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
  ++dcmdump_level;
}

static void dcmdump_sequenceoffragments(DICM_UNUSED struct _writer *writer,
                                        const struct _dataelement *de) {
  if (dcmdump_level) printf("%*c", 1 << dcmdump_level, ' ');
  printf("(%04x,%04x) %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr), de->vl);
  ++dcmdump_level;
}

static void dcmdump_dataelement(struct _writer *writer,
                                const struct _dataelement *de) {
  if (!first_dataelement) {
    printf("\n");
    printf("# Dicom-Data-Set\n");
    printf("# Used TransferSyntax: RLE Lossless\n");
    first_dataelement = 1;
  }
  write_dataelement(writer, de);
}

const struct _writer_ops dcmdump_writer = {
    .write_file_preamble = dcmdump_file_preamble,
    .write_prefix = dcmdump_prefix,
    .write_filemetaelement = dcmdump_filemetaelement,
    .write_dataelement = dcmdump_dataelement,
    .write_sequenceofitems = dcmdump_sequenceofitems,
    .write_sequenceoffragments = dcmdump_sequenceoffragments,
    .write_item = dcmdump_item,
    .write_bot = dcmdump_bot,
    .write_fragment = dcmdump_fragment,
    .write_end_item = dcmdump_end_item,
    .write_end_sq = dcmdump_end_sq,
    .write_end_frags = dcmdump_end_frags,
};
