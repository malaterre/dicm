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
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include "dicm-io.h"

#include "dicm-log.h"
#include "dicm-private.h"

#include <assert.h> /* assert */
#include <stdio.h>  /* fopen */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

struct _file {
  struct dicm_io io;
  /* data */
  FILE *stream;
  const char *filename;
};

#if 0
// https://stackoverflow.com/questions/58670828/is-there-a-way-to-rewind-stdin-in-c
static bool is_stream_seekable(FILE* stream) {
  if (fseek(stdin, 0L, SEEK_SET)) {
    return false;
  }
  return true;
}
#endif

static DICM_CHECK_RETURN int _file_destroy(void *self_) DICM_NONNULL;
static DICM_CHECK_RETURN int _file_read(void *self_, void *buf,
                                        size_t size) DICM_NONNULL;
static DICM_CHECK_RETURN int _file_seek(void *self_,
                                        io_offset off) DICM_NONNULL;
static DICM_CHECK_RETURN int _file_tell(void *self_,
                                        io_offset *off) DICM_NONNULL;
static DICM_CHECK_RETURN int _file_write(void *self_, void const *buf,
                                         size_t size) DICM_NONNULL;

static struct io_vtable const g_vtable = {
    /* object interface */
    .object = {.fp_destroy = _file_destroy},
    /* io interface */
    .io = {.fp_read = _file_read,
           .fp_seek = _file_seek,
           .fp_tell = _file_tell,
           .fp_write = _file_write}};

int dicm_io_file_create(struct dicm_io **pself, const char *filename,
                        int mode) {
  int errsv = 0;
  struct _file *self = (struct _file *)malloc(sizeof(*self));
  errsv = errno; /* ENOMEM */
  if (self) {
    *pself = &self->io;
    self->io.vtable = &g_vtable;
    assert(mode == DICM_IO_READ || mode == DICM_IO_WRITE);
    FILE *stream =
        mode == DICM_IO_READ ? fopen(filename, "rb") : fopen(filename, "wb");
    errsv = errno;
    self->stream = stream;
    self->filename = filename;
    // TODO:
    // https://en.cppreference.com/w/c/io/setvbuf
    if (stream) {
      return 0;
    }
  }
  // log_errno(debug, errsv); // FIXME
  *pself = NULL;
  return errsv;
}

int dicm_io_stream_create(struct dicm_io **pself, int mode) {
  int errsv = 0;
  struct _file *self = (struct _file *)malloc(sizeof(*self));
  errsv = errno; /* ENOMEM */
  if (self) {
    *pself = &self->io;
    self->io.vtable = &g_vtable;
    assert(mode == DICM_IO_READ || mode == DICM_IO_WRITE);
    FILE *stream = mode == DICM_IO_READ ? stdin : stdout;
    self->stream = stream;
    self->filename = NULL;
    return 0;
  }
  // log_errno(debug, errsv); // FIXME
  *pself = NULL;
  return errsv;
}

int _file_destroy(void *const self_) {
  int errsv = 0;
  struct _file *self = (struct _file *)self_;
  if (self->filename) {
    /* it is an error only if the stream was already opened */
    if (self->stream && fclose(self->stream)) {
      /* some failure */
      errsv = errno;
    }
  }
  free(self);
  return errsv;
}

int _file_read(void *const self_, void *buf, size_t size) {
  struct _file *self = (struct _file *)self_;
  const size_t read = fread(buf, 1, size, self->stream);
  /* fread() does not distinguish between end-of-file and error, and callers
   * must use feof(3) and ferror(3) to determine which occurred. */
  if (read != size) {
    // TODO
    return 1;
  }
  return 0;
}

int _file_seek(void *const self_, io_offset off) { return 1; }
int _file_tell(void *const self_, io_offset *off) {
  *off = 0;
  return 1;
}

int _file_write(void *const self_, void const *buf, size_t size) {
  struct _file *self = (struct _file *)self_;
  const size_t write = fwrite(buf, 1, size, self->stream);
  if (write != size) {
    // TODO
    return 1;
  }
  return 0;
}

#if 0
static bool fsrc_open(struct _src *src, const char *fspec) {
  if (src == NULL) return false;
  FILE *file = fopen(fspec, "rb");
  // TODO:
  // https://en.cppreference.com/w/c/io/setvbuf
  src->data = file;
  if (file == NULL) {
    log_errno(debug);
    return false;
  }
  return true;
}

static bool fsrc_close(struct _src *src) {
  if (src == NULL || src->data == NULL) return false;
  FILE *stream = src->data;
  assert(ferror(stream) == 0);
  const bool ret = fclose(stream) == 0;
  src->data = NULL;
  return ret;
}

static size_t fsrc_read(struct _src *src, void *buf, size_t bsize) {
  if (src == NULL || src->data == NULL) return 0;
  assert(ferror(src->data) == 0);
  const size_t read = fread(buf, 1, bsize, src->data);
  /* fread() does not distinguish between end-of-file and error, and callers
   * must use feof(3) and ferror(3) to determine which occurred. */
  return read;
}

static bool fsrc_at_end(struct _src *src) {
  assert(ferror(src->data) == 0);
  return feof(src->data);
}

static bool fsrc_seek(struct _src *src, offset_t offset) {
  assert(ferror(src->data) == 0);
  const bool ret = fseek(src->data, offset, SEEK_CUR) == 0;
  return ret;
}

static offset_t fsrc_tell(struct _src *src) {
  offset_t ret = ftell(src->data);
  if (ret == (offset_t)-1) {
    log_errno(debug);
  }
  return ret;
}

static bool fdst_open(struct _dst *dst, const char *fspec) {
  FILE *file = fopen(fspec, "wb");
  dst->data = file;
  return true;
}

static bool fdst_close(struct _dst *dst) {
  fclose(dst->data);
  return true;
}

static size_t fdst_write(struct _dst *dst, const void *buf, size_t bsize) {
  const size_t write = fwrite(buf, 1, bsize, dst->data);
  if (write != bsize) {
    return bsize;
  }
  return bsize;
}

const struct _src_ops fsrc_ops = {
    .open = fsrc_open,
    .close = fsrc_close,
    .read = fsrc_read,
    .at_end = fsrc_at_end,
    .seek = fsrc_seek,
    .tell = fsrc_tell,
};

const struct _dst_ops fdst_ops = {
    .open = fdst_open,
    .close = fdst_close,
    .write = fdst_write,
};

// struct _src fsrc = {
//  .ops = &fsrc_ops,
//  .data = NULL
//};
//
// struct _dst fdst = {
//  .ops = &fdst_ops,
//  .data = NULL
//};
#endif
