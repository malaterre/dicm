// SPDX-License-Identifier: LGPLv3

#include "dicm.h"

#include "dicm-de.h"
#include "dicm-log.h"
#include "dicm-private.h"
#include "reader.h"
#include "writer.h"

extern struct _log dlog;

#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include <stdio.h>  /* fopen */
#include <stdlib.h> /* EXIT_SUCCESS */

void process_writer(struct dicm_reader *reader, struct dicm_writer *writer) {
  char buf[16384 /*4096*/];  // FIXME
  size_t size = sizeof buf;
  struct dicm_attribute da;
  struct dicm_attribute sq;
  int fragment;
  int item;
  char encoding[64];
  while (dicm_reader_hasnext(reader)) {
    int next = dicm_reader_next(reader);
    switch (next) {
      case kStartAttribute:
        dicm_reader_get_attribute(reader, &da);
        dicm_writer_write_start_attribute(writer, &da);
        break;

      case kEndAttribute:
        dicm_writer_write_end_attribute(writer);
        break;

      case kValue:
        /* FIXME: need a while + size handling */
        size = sizeof buf;
        dicm_reader_get_value(reader, buf, &size);
        dicm_writer_write_value(writer, buf, size);
        break;

      case kStartFragment:
        dicm_reader_get_fragment(reader, &fragment);
        dicm_writer_write_start_fragment(writer, fragment);
        break;

      case kEndFragment:
        dicm_writer_write_end_fragment(writer);
        break;

      case kStartItem:
        dicm_reader_get_item(reader, &item);
        dicm_writer_write_start_item(writer, item);
        break;

      case kEndItem:
        dicm_writer_write_end_item(writer);
        break;

      case kStartSequence:
        dicm_reader_get_attribute(reader, &sq);
        dicm_writer_write_start_sequence(writer, &sq);
        break;

      case kEndSequence:
        dicm_writer_write_end_sequence(writer);
        break;

      case kStartModel:
        dicm_reader_get_encoding(reader, encoding, sizeof encoding);
        dicm_writer_write_start_model(writer, encoding);
        break;

      case kEndModel:
        dicm_writer_write_end_model(writer);
        break;

      default:
        printf("wotsit: %d\n", next);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) return EXIT_FAILURE;
  const char *options = argv[1];
  const char *filename = argv[1];
  if (argc > 2) filename = argv[2];
  set_global_logger(&dlog);

  struct dicm_io *src;
  struct dicm_io *dst;
  dicm_io_file_create(&src, filename, DICM_IO_READ);
  dicm_io_file_create(&dst, "output.dcm", DICM_IO_WRITE);

  struct dicm_reader *reader;
  dicm_reader_utf8_create(&reader, src);

#if 0
  if (strcmp(options, "fme") == 0) {
    dicm_sreader_stream_filemetaelements(sreader, true);
  } else if (strcmp(options, "gl") == 0) {
    dicm_sreader_group_length(sreader, true);
  } else if (strcmp(options, "all") == 0) {
    dicm_sreader_stream_filemetaelements(sreader, true);
    dicm_sreader_group_length(sreader, true);
  }
#endif

  struct dicm_writer *writer;
  dicm_json_writer_create(&writer);
  process_writer(reader, writer);

  /* cleanup */
  object_destroy(reader);
  object_destroy(writer);
  object_destroy(src);
  object_destroy(dst);

  return EXIT_SUCCESS;
}