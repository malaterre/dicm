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
  void (*print_dataelement)(const struct _dataelement *de);
  void (*print_item)();
};

static const struct _writer default_writer = {
  .print_file_preamble = print_file_preamble,
  .print_prefix = print_prefix,
  .print_dataelement = print_dataelement,
  .print_item = print_item,
};

void process_writer(const struct _writer *writer, dicm_sreader_t *sreader)
{
  struct _dataelement *de;
  const char * buf;
  while (dicm_sreader_hasnext(sreader)) {
    int next = dicm_sreader_next(sreader);
    switch (next) {
      case kStartInstance:
        break;

      case kFilePreamble:
        if( (buf  = dicm_sreader_get_file_preamble(sreader))) writer->print_file_preamble(buf);
        break;

      case kPrefix:
        if((buf  = dicm_sreader_get_prefix(sreader))) writer->print_prefix(buf);
        break;

      case kFileMetaElement:
        if ((de = dicm_sreader_get_dataelement(sreader))) writer->print_dataelement(de);
        break;

      case kDataElement:
        if ((de = dicm_sreader_get_dataelement(sreader))) writer->print_dataelement(de);
        break;

    case kItem:
        writer->print_item();
        break;

      case kEndInstance:
        break;
    }
  }
 
}

int main(int argc, char *argv[]) {
  if( argc < 2 ) return EXIT_FAILURE;
  const char * filename = argv[1];
  set_global_logger(&dlog);

  src_t fsrc;
  dst_t fdst;
  dicm_sreader_t *sreader;

  fsrc.ops = &fsrc_ops;
  fdst.ops = &fdst_ops;

  fsrc.ops->open(&fsrc, filename);
  fdst.ops->open(&fdst, "output.dcm");

  sreader = dicm_sreader_init(&ansi, &fsrc);
  process_writer(&default_writer, sreader);
  dicm_sreader_fini(sreader);

  fsrc.ops->close(&fsrc);
  fdst.ops->close(&fdst);

  return EXIT_SUCCESS;
}
