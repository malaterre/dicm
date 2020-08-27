#pragma once

#include <sys/types.h> /* off_t */

typedef off_t offset_t;

struct _src {
  const struct _src_ops *ops;
  void *data;
};

struct _src_ops {
  int (*open)(struct _src *src, const char *fspec);
  int (*close)(struct _src *src);
  int (*read)(struct _src *src, char *buf, size_t bsize);
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
  int (*write)(struct _dst *dst, char *buf, size_t bsize);
};

/** stream reader */
struct _dicm_sreader {
  struct _src *src;
//    int (*init)(struct _dicm_sreader *self, struct _src *src);
//    int (*fini)(struct _dicm_sreader *self, struct _src *src);
//
//    int (*next)(struct _dicm_sreader *self) /* throws XMLStreamException */;
//    int (*has_next)(struct _dicm_sreader *self) /* throws XMLStreamException */;

//    public String getText();
//    public String getLocalName();
//    public String getNamespaceURI();
};
