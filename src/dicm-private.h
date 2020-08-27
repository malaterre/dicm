#pragma once

#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200808L

#include "parser.h"
#include <sys/types.h> /* off_t */

typedef off_t offset_t;

struct _src {
  const struct _src_ops *ops;
  void *data;
};

struct _src_ops {
  int (*open)(struct _src *src, const char *fspec);
  int (*close)(struct _src *src);
  size_t (*read)(struct _src *src, void *buf, size_t bsize);
  int (*seek)(struct _src *src, offset_t offset);
  offset_t (*tell)(struct _src *src);
};

/** dest */
struct _dst {
  const struct _dst_ops *ops;
  void *data;
};

struct _dst_ops {
  int (*open)(struct _dst *dst, const char *fspec);
  int (*close)(struct _dst *dst);
  /**
   * bsize in bytes
   */
  size_t (*write)(struct _dst *dst, void *buf, size_t bsize);
};

enum state {
  kStartInstance = 0,
  // http://dicom.nema.org/medical/dicom/current/output/chtml/part10/chapter_7.html#table_7.1-1
  kFilePreamble,
  kPrefix,
  kFileMetaElement,
  kDataElement,
  kEndInstance
};

/** stream reader */
struct _dicm_sreader {
  struct _src *src;
  struct _dataelement dataelement;
  enum state current_state;
  char buffer[4096];  // FIXME remove me
  size_t bufsizemax;  // 4096;
  int bufpos;
};
