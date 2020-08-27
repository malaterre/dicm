typedef off_t offset_t;

struct _src_ops {
  bool (*open)(void *self, char *fspec);
  bool (*close)(void *self);
  size_t (*read)(void *self, void *buff, size_t bsize);
};

struct _src {
  const struct _src_ops *ops;
};

struct _dst_ops {
  bool (*open)(void *self, char *fspec);
  bool (*close)(void *self);
  size_t (*write)(void *self, void *buff, size_t bsize);
};

struct _dst {
  const struct _dst_ops *ops;
};
