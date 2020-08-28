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

#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200808L

#include "dicm.h"

#include "dicm-private.h"
#include "dicm-log.h"
#include "dicm-parser.h"

extern struct _log dlog;

#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include <stdio.h>  /* fopen */
#include <stdlib.h> /* EXIT_SUCCESS */

static int fsrc_open(struct _src *src, const char *fspec) {
  FILE *file = fopen(fspec, "rb");
  src->data = file;
  if( file == NULL ) {
    log_errno();
    return -errno;
  }
  return 0;
}

static int fsrc_close(struct _src *src) { return fclose(src->data); }

static size_t fsrc_read(struct _src *src, void *buf, size_t bsize) {
  assert(bsize != (size_t)-1);
  const size_t read = fread(buf, 1, bsize, src->data);
  /* fread() does not distinguish between end-of-file and error, and callers
   * must use feof(3) and ferror(3) to determine which occurred. */
  if (read != bsize) {
    if (feof(src->data) != 0) {
      return (size_t)-1;
    }
    return read;
  }
  assert(bsize == read);
  return bsize;
}

static int fsrc_seek(struct _src *src, offset_t offset) {
  return fseeko(src->data, offset, SEEK_CUR);
}

static offset_t fsrc_tell(struct _src *src) { return ftello(src->data); }

static int fdst_open(struct _dst *dst, const char *fspec) {
  FILE *file = fopen(fspec, "wb");
  dst->data = file;
  return errno;
}

static int fdst_close(struct _dst *dst) {
  fclose(dst->data);
  return errno;
}

static size_t fdst_write(struct _dst *dst, void *buf, size_t bsize) {
  const size_t write = fwrite(buf, 1, bsize, dst->data);
  if (write != bsize) {
    return bsize;
  }
  return bsize;
}

static const struct _src_ops fsrc_ops = {
    .open = fsrc_open,
    .close = fsrc_close,
    .read = fsrc_read,
    .seek = fsrc_seek,
    .tell = fsrc_tell,
};

static const struct _dst_ops fdst_ops = {
    .open = fdst_open,
    .close = fdst_close,
    .write = fdst_write,
};

int main(__maybe_unused int argc, __maybe_unused char *argv[]) {
  set_global_logger(&dlog);

  src_t fsrc;
  dst_t fdst;
  dicm_sreader_t *sreader;

  fsrc.ops = &fsrc_ops;

  fdst.ops = &fdst_ops;

  fsrc.ops->open(&fsrc, "input.dcm");
  fdst.ops->open(&fdst, "output.dcm");

  sreader = dicm_sreader_init(&fsrc);
  struct _dataelement de = {0};
  while (dicm_sreader_hasnext(sreader)) {
    int next = dicm_sreader_next(sreader);
    switch (next) {
      case kStartInstance:
        break;

      case kFilePreamble:
        break;

      case kPrefix:
        break;

      case kFileMetaElement:
        dicm_sreader_get_dataelement(sreader, &de);
        print_dataelement(&de);
        break;

      case kDataElement:
        dicm_sreader_get_dataelement(sreader, &de);
        print_dataelement(&de);
        break;

      case kEndInstance:
        break;
    }
  }
  dicm_sreader_fini(sreader);

  fsrc.ops->close(&fsrc);
  fdst.ops->close(&fdst);

  return EXIT_SUCCESS;
}
