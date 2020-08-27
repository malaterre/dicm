typedef struct {
  int (*open)(void *self, char *fspec);
  int (*close)(void *self);
  int (*read)(void *self, void *buff, size_t max_sz, size_t *p_act_sz);
  int (*write)(void *self, void *buff, size_t max_sz, size_t *p_act_sz);
  // And data goes here.
} tCommClass;
