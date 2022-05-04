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
  int res;

  const size_t buflen = sizeof buf;

  while (dicm_reader_hasnext(reader)) {
    int next = dicm_reader_next_event(reader);
    switch (next) {
      case EVENT_ATTRIBUTE:
        res = dicm_reader_get_attribute(reader, &da);
        assert(res == 0);
        res = dicm_writer_write_attribute(writer, &da);
        assert(res == 0);
        break;

      case EVENT_VALUE:
        res = dicm_reader_get_value_length(reader, &size);
        assert(res == 0);
        res = dicm_writer_write_value_length(writer, size);
        assert(res == 0);
        /* do/while loop trigger at least one event (even in the case where
         * value_length is exactly 0) */
        do {
          const size_t len = size < buflen ? size : buflen;
          res = dicm_reader_read_value(reader, buf, len);
          assert(res == 0);
          res = dicm_writer_write_value(writer, buf, len);
          assert(res == 0);
          size -= len;
        } while (size != 0);
        break;

      case EVENT_FRAGMENT:
        res = dicm_writer_write_fragment(writer);
        assert(res == 0);
        break;

      case EVENT_START_ITEM:
        res = dicm_writer_write_start_item(writer);
        assert(res == 0);
        break;

      case EVENT_END_ITEM:
        res = dicm_writer_write_end_item(writer);
        assert(res == 0);
        break;

      case EVENT_START_SEQUENCE:
        res = dicm_writer_write_start_sequence(writer);
        assert(res == 0);
        break;

      case EVENT_END_SEQUENCE:
        res = dicm_writer_write_end_sequence(writer);
        assert(res == 0);
        break;

      case EVENT_START_DATASET:
        res = dicm_reader_get_encoding(reader, encoding, sizeof encoding);
        assert(res == 0);
        res = dicm_writer_write_start_dataset(writer, encoding);
        assert(res == 0);
        break;

      case EVENT_END_DATASET:
        res = dicm_writer_write_end_dataset(writer);
        assert(res == 0);
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
