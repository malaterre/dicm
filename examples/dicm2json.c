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
  /* attribute */
  struct dicm_attribute da;
  /* value */
  char buf[4096];
  size_t size;
  /* specific charactet set */
  char encoding[64];

  /* FIXME: let's be lazy with base64 */
  const size_t len3 = (sizeof buf / 3u) * 3u;
  assert(len3 <= sizeof buf && len3 % 3 == 0);

  while (dicm_reader_hasnext(reader)) {
    int next = dicm_reader_next_event(reader);
    switch (next) {
      case EVENT_ATTRIBUTE:
        dicm_reader_get_attribute(reader, &da);
        dicm_writer_write_attribute(writer, &da);
        break;

      case EVENT_VALUE:
        dicm_reader_get_value_length(reader, &size);
        dicm_writer_write_value_length(writer, size);
        /* do/while loop trigger at least one event (even in the case where
         * value_length is exactly 0) */
        do {
          const size_t len = size < len3 ? size : len3;
          dicm_reader_read_value(reader, buf, len);
          dicm_writer_write_value(writer, buf, len);
          size -= len;
        } while (size != 0);
        break;

      case EVENT_FRAGMENT:
        dicm_writer_write_fragment(writer);
        break;

      case EVENT_START_ITEM:
        dicm_writer_write_start_item(writer);
        break;

      case EVENT_END_ITEM:
        dicm_writer_write_end_item(writer);
        break;

      case EVENT_START_SEQUENCE:
        dicm_writer_write_start_sequence(writer);
        break;

      case EVENT_END_SEQUENCE:
        dicm_writer_write_end_sequence(writer);
        break;

      case EVENT_START_DATASET:
        dicm_reader_get_encoding(reader, encoding, sizeof encoding);
        dicm_writer_write_start_dataset(writer, encoding);
        break;

      case EVENT_END_DATASET:
        dicm_writer_write_end_dataset(writer);
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
  dicm_io_file_create(&dst, "output.json", DICM_IO_WRITE);

  struct dicm_reader *reader;
  dicm_reader_utf8_create(&reader, src);

  struct dicm_writer *writer;
  dicm_json_writer_create(&writer, dst);
  process_writer(reader, writer);

  /* cleanup */
  object_destroy(reader);
  object_destroy(writer);
  object_destroy(src);
  object_destroy(dst);
  object_destroy(log);

  return EXIT_SUCCESS;
}
