#include "dicm.h"
#include "parser.h"

#include <string.h> /* memset */
#include <assert.h> /* assert */

int dicm_sreader_init(struct _dicm_sreader *sreader, struct _src *src)
{
  sreader->src = src;
  sreader->current_state = kStartInstance;
  memset(sreader->buffer, 0, sizeof sreader->buffer);;
  sreader->bufsizemax = sizeof sreader->buffer;
  sreader->bufpos = -1;
  sreader->dataelement.tag = 0;
  return 0;
}

int dicm_sreader_hasnext(struct _dicm_sreader *sreader)
{
  return sreader->current_state != kEndInstance;
}

static inline int getbufsize(struct _dicm_sreader *sreader)
{
  assert( sreader->bufpos != -1 );
  return (int)sreader->bufsizemax - sreader->bufpos;
}

int dicm_sreader_next(struct _dicm_sreader *sreader)
{
  struct _src *src = sreader->src;
  char *buf = sreader->buffer;
  int current_state = sreader->current_state;
  struct _dataelement *de = &sreader->dataelement;

    size_t bufsize;
    switch (current_state)
    {
        case kStartInstance:
        /* Do something with input and set current_state */
        // get new input:
        bufsize = 128;
        src->ops->read(src, buf, bufsize);
        sreader->current_state = kFilePreamble;
        break;

        case kFilePreamble:
        bufsize = 4;
        src->ops->read(src, buf, bufsize);
        sreader->current_state = kPrefix;
        break;

        case kPrefix:
        bufsize = sizeof(struct _dataelement);
        src->ops->read(src, buf, bufsize);
        read_explicit1( src, de );
        read_explicit2( src, de );
        sreader->current_state = kFileMetaElement;
        break;

        case kFileMetaElement:
        sreader->current_state = kDataElement;
        break;

        case kDataElement:
        sreader->current_state = kEndInstance;
        break;

        case kEndInstance:
        /* Do something different and set current_state */
        break;
    }
        return sreader->current_state;
}

int dicm_sreader_get_dataelement(struct _dicm_sreader *sreader, struct _dataelement *de)
{
  if( sreader->current_state != kFileMetaElement ) return kError;
  return kSuccess;
}

int dicm_sreader_fini(struct _dicm_sreader *sreader)
{
  return 0;
}
