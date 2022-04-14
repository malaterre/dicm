// SPDX-License-Identifier: LGPLv3

#include "dicm.h"

#include "dicm-log.h"
#include "dicm-private.h"
#include "dicm-public.h"
#include "reader.h"
#include "writer.h"

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
        dicm_reader_get_value_length(reader, &size);
#if 0
        const int n = size / sizeof buf;
        for (int i = 0; i < n; ++i) {
          dicm_reader_read_value(reader, buf, sizeof buf);
          dicm_writer_write_value(writer, buf, sizeof buf);
        }
        const size_t to_read = size % sizeof buf;
        if (to_read) {
          dicm_reader_read_value(reader, buf, to_read);
          dicm_writer_write_value(writer, buf, to_read);
        }
#else
        do {
          const size_t to_read = size < sizeof buf ? size : sizeof buf;
          dicm_reader_read_value(reader, buf, to_read);
          dicm_writer_write_value(writer, buf, to_read);
          size -= to_read;

        } while (size != 0);
#endif
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

  struct dicm_log *log;
  dicm_log_create(&log, stderr);
  dicm_log_set_global(log);

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

  dicm_log_msg(2, "log test");

  object_destroy(log);

  return EXIT_SUCCESS;
}
