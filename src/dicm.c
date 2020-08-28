#include "dicm.h"

#include "dicm-private.h"
#include "parser.h"

#include <assert.h> /* assert */
#include <string.h> /* memset */
#include <stdlib.h> /* malloc/free */

/** stream reader */
struct _dicm_sreader {
  struct _src *src;
  struct _dataelement dataelement;
  enum state current_state;
  char buffer[128 /*4096*/];  // Minimal amount of memory (preamble is the bigest one ?)
  size_t bufsizemax;  // = sizeof buffer
};

struct _dicm_sreader*
dicm_sreader_init(struct _src *src) {
  struct _dicm_sreader *sreader = malloc(sizeof *sreader);
  sreader->src = src;
  sreader->current_state = kStartInstance;
  memset(sreader->buffer, 0, sizeof sreader->buffer);
  sreader->bufsizemax = sizeof sreader->buffer;
  sreader->dataelement.tag = 0;
  return sreader;
}

int dicm_sreader_hasnext(struct _dicm_sreader *sreader) {
  return sreader->current_state != kEndInstance;
}

int dicm_sreader_next(struct _dicm_sreader *sreader) {
  struct _src *src = sreader->src;
  char *buf = sreader->buffer;
  int current_state = sreader->current_state;
  struct _dataelement *de = &sreader->dataelement;

  size_t bufsize;
  switch (current_state) {
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
      src->ops->read(src, buf, 4 + 2);
      read_explicit1(de, buf, 4 + 2);
      {
        size_t llen = get_explicit2_len(de);
        src->ops->read(src, buf, llen);
        read_explicit2(de, buf, llen);
      }
      src->ops->seek(src, de->vl);
      if (get_group(de->tag) == 0x2)
        sreader->current_state = kFileMetaElement;
      else
        assert(0);
      break;

    case kFileMetaElement:
      src->ops->read(src, buf, 4 + 2);
      read_explicit1(de, buf, 4 + 2);
      {
        size_t llen = get_explicit2_len(de);
        src->ops->read(src, buf, llen);
        read_explicit2(de, buf, llen);
      }
      src->ops->seek(src, de->vl);
      if (get_group(de->tag) == 0x2)
        sreader->current_state = kFileMetaElement;
      else if (get_group(de->tag) >= 0x8)
        sreader->current_state = kDataElement;
      break;

    case kDataElement: {
      size_t d = src->ops->read(src, buf, 4 + 2);
      if (d == (size_t)-1) {
        sreader->current_state = kEndInstance;
      } else {
        read_explicit1(de, buf, 4 + 2);
        size_t llen = get_explicit2_len(de);
        src->ops->read(src, buf, llen);
        read_explicit2(de, buf, llen);
        src->ops->seek(src, de->vl);
        if (get_group(de->tag) >= 0x8) sreader->current_state = kDataElement;
      }
    } break;

    case kEndInstance:
      /* Do something different and set current_state */
      break;
  }
  return sreader->current_state;
}

int dicm_sreader_get_dataelement(struct _dicm_sreader *sreader,
                                 struct _dataelement *de) {
  if (sreader->current_state != kFileMetaElement &&
      sreader->current_state != kDataElement)
    return kError;
  memcpy(de, &sreader->dataelement, sizeof(struct _dataelement));
  return kSuccess;
}

int dicm_sreader_fini(struct _dicm_sreader *sreader) {
 free(sreader);
 return 0; 
}
