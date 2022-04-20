// SPDX-License-Identifier: LGPLv3

#include "dicm-public.h"

#include "dicm-log.h"
#include "dicm-reader.h"
#include "dicm-writer.h"

#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include <stdio.h>  /* fopen */
#include <stdlib.h> /* EXIT_SUCCESS */

void process_writer(struct dicm_reader *reader, struct dicm_writer *writer) {
  char buf[4096];
  size_t size;
  struct dicm_attribute da;
  struct dicm_attribute sq;
  int fragment = -1;
  int item = 45;
  char encoding[64];

  /* let's be lazy with base64 */
  const size_t len3 = (sizeof buf / 3u) * 3u;
  assert(len3 <= sizeof buf && len3 % 3 == 0);

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
        if (true /*fragment == -1*/) {
          dicm_reader_get_value_length(reader, &size);
          /* do/while loop trigger at least one event (even in the case where
           * value_length is exactly 0) */
          do {
            const size_t len = size < len3 ? size : len3;
            dicm_reader_read_value(reader, buf, len);
            dicm_writer_write_value(writer, buf, len);
            size -= len;
          } while (size != 0);
        }
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

      case kStartFragments:
        fragment = -1;
        dicm_reader_get_attribute(reader, &da);
        dicm_writer_write_start_attribute(writer, &da);
        break;

      case kEndFragments:
        fragment = -1;
        dicm_writer_write_end_attribute(writer);
        break;

      case kStartSequence:
        item = 0;
        dicm_reader_get_attribute(reader, &sq);
        dicm_writer_write_start_sequence(writer, &sq);
        break;

      case kEndSequence:
        item = 0;
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
        assert(0);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) return EXIT_FAILURE;
  const char *filename = argv[1];

  struct dicm_log *log;
  dicm_log_create(&log, stderr);
  dicm_log_set_global(log);

  struct dicm_io *src;
  struct dicm_io *dst;
  dicm_io_file_create(&src, filename, DICM_IO_READ);
  dicm_io_file_create(&dst, "output.dcm", DICM_IO_WRITE);

  struct dicm_reader *reader;
  dicm_reader_utf8_create(&reader, src);

  struct dicm_writer *writer;
  dicm_writer_utf8_create(&writer, dst);
  process_writer(reader, writer);

  /* cleanup */
  object_destroy(reader);
  object_destroy(writer);
  object_destroy(src);
  object_destroy(dst);
  object_destroy(log);

  return EXIT_SUCCESS;
}