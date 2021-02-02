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
#include "writer.h"

extern struct _log dlog;
extern const struct _src_ops fsrc_ops;
extern const struct _dst_ops fdst_ops;
extern struct _mem ansi;

extern const struct _writer_ops default_writer;
extern const struct _writer_ops event_writer;
extern const struct _writer_ops dcmdump_writer;
extern const struct _writer_ops copy_writer;

#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include <stdio.h>  /* fopen */
#include <stdlib.h> /* EXIT_SUCCESS */

void process_writer(struct _writer *writer, dicm_sreader_t *sreader) {
  struct _dicm_filepreamble filepreamble;
  struct _dicm_prefix prefix;
  struct _filemetaelement fme;
  struct _dataelement de;
  while (dicm_sreader_hasnext(sreader)) {
    int next = dicm_sreader_next(sreader);
    switch (next) {
      case kStartFileMetaInformation:
        writer->ops->print_start_fmi(writer);
        break;

      case kFilePreamble:
        if (dicm_sreader_get_file_preamble(sreader, &filepreamble))
          writer->ops->print_file_preamble(writer, &filepreamble);
        break;

      case kDICOMPrefix:
        if (dicm_sreader_get_prefix(sreader, &prefix))
          writer->ops->print_prefix(writer, &prefix);
        break;

      case kFileMetaInformationGroupLength:
        writer->ops->print_fmi_gl(writer, 0);
        break;

      case kFileMetaElement:
        if (dicm_sreader_get_filemetaelement(sreader, &fme))
          writer->ops->print_filemetaelement(writer, &fme);
        break;

      case kEndFileMetaInformation:
        writer->ops->print_end_fmi(writer);
        break;

      case kDataElement:
        if (dicm_sreader_get_dataelement(sreader, &de))
          writer->ops->print_dataelement(writer, &de);
        break;

      case kGroupLengthDataElement:
        writer->ops->print_group_gl(writer, &de);
        break;

      case kEndGroupDataElement:
        writer->ops->print_end_group(writer);
        break;

      case kSequenceOfItems:
        if (dicm_sreader_get_dataelement(sreader, &de))
          writer->ops->print_sequenceofitems(writer, &de);
        break;

      case kSequenceOfFragments:
        if (dicm_sreader_get_dataelement(sreader, &de))
          writer->ops->print_sequenceoffragments(writer, &de);
        break;

      case kItem:
        if (dicm_sreader_get_dataelement(sreader, &de))
          writer->ops->print_item(writer, &de);
        break;

      case kBasicOffsetTable:
        if (dicm_sreader_get_dataelement(sreader, &de))
          writer->ops->print_bot(writer, &de);
        break;

      case kFragment:
        if (dicm_sreader_get_dataelement(sreader, &de))
          writer->ops->print_fragment(writer, &de);
        break;

      case kItemDelimitationItem:
        if (dicm_sreader_get_dataelement(sreader, &de))
          writer->ops->print_end_item(writer, &de);
        break;

      case kSequenceOfItemsDelimitationItem:
        if (dicm_sreader_get_dataelement(sreader, &de))
          writer->ops->print_end_sq(writer, &de);
        break;

      case kSequenceOfFragmentsDelimitationItem:
        if (dicm_sreader_get_dataelement(sreader, &de))
          writer->ops->print_end_frags(writer, &de);
        break;

      default:
        printf("wotsit: %d\n", next);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) return EXIT_FAILURE;
  const char *options = argv[1];
  const char *filename = argv[2];
  if (argc == 2) filename = argv[1];
  set_global_logger(&dlog);

  src_t fsrc;
  dst_t fdst;
  struct _writer writer;
  dicm_sreader_t *sreader;

  fsrc.ops = &fsrc_ops;
  fdst.ops = &fdst_ops;

  fsrc.ops->open(&fsrc, filename);
  fdst.ops->open(&fdst, "output.dcm");

  sreader = dicm_sreader_init(&ansi);
  dicm_sreader_set_src(sreader, &fsrc);
  if( strcmp(options, "fme") == 0 ) 
    dicm_sreader_stream_filemetaelements(sreader, true);
  else if( strcmp(options, "gl") == 0 ) 
    dicm_sreader_group_length(sreader, true);
  else if( strcmp(options, "all") == 0 )  {
    dicm_sreader_stream_filemetaelements(sreader, true);
    dicm_sreader_group_length(sreader, true);
}
  /*  if (!dicm_sreader_read_meta_info(sreader)) {
      return EXIT_FAILURE;
    }*/

  // writer.ops = &dcmdump_writer;
  writer.ops = &event_writer;
  // writer.ops = &default_writer;
  // writer.ops = &copy_writer;
  writer.sreader = sreader;
  writer.dst = &fdst;
  process_writer(&writer, sreader);
  dicm_sreader_fini(sreader);

  fsrc.ops->close(&fsrc);
  fdst.ops->close(&fdst);

  return EXIT_SUCCESS;
}
