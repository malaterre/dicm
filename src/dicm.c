#include "dicm.h"

int dicm_sreader_init(struct _dicm_sreader *sreader, struct _src *src)
{
  sreader->src = src;
  return 0;
}

int dicm_sreader_hasnext(struct _dicm_sreader *sreader)
{
  return 0;
}

int dicm_sreader_next(struct _dicm_sreader *sreader)
{
  return 0;
}

int dicm_sreader_fini(struct _dicm_sreader *sreader)
{
  return 0;
}
