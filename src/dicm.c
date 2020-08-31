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

#include "dicm.h"

#include "dicm-private.h"
#include "dicm-parser.h"

#include <assert.h> /* assert */
#include <stdlib.h> /* malloc/free */
#include <string.h> /* memset */

/** stream reader */
struct _dicm_sreader {
  struct _mem *mem;
  struct _src *src;
  struct _dataelement dataelement;
  enum state current_state;
  char buffer[128 /*4096*/];  // Minimal amount of memory (preamble is the
                              // bigest one ?)
  size_t bufsizemax;          // = sizeof buffer
};

struct _dicm_sreader *dicm_sreader_init(struct _mem *mem, struct _src *src) {
  struct _dicm_sreader *sreader = mem->ops->alloc(mem, sizeof *sreader);
  sreader->mem = mem;
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
      read_explicit(src, de);
      if (dicm_de_get_group(de) == 0x2)
        sreader->current_state = kFileMetaElement;
      else
        assert(0);
      break;

    case kFileMetaElement:
      read_explicit(src, de);
      if (dicm_de_get_group(de) == 0x2)
        sreader->current_state = kFileMetaElement;
      else if (dicm_de_get_group(de) >= 0x8)
        sreader->current_state = kDataElement;
      break;

    case kDataElement: {
      if (read_explicit(src, de) == -1) {
        sreader->current_state = kEndInstance;
      } else {
        if (dicm_de_get_group(de) >= 0x8) sreader->current_state = kDataElement;
      }
    } break;

    case kEndInstance:
      /* Do something different and set current_state */
      break;
  }
  return sreader->current_state;
}

struct _dataelement * dicm_sreader_get_dataelement(struct _dicm_sreader *sreader
                                 ) {
  if (sreader->current_state != kFileMetaElement &&
      sreader->current_state != kDataElement)
    return NULL; //-kDicmInvalidArgument;
  return &sreader->dataelement;
//  memcpy(de, &sreader->dataelement, sizeof(struct _dataelement));
//  return 0;
}

int dicm_sreader_fini(struct _dicm_sreader *sreader) {
  sreader->mem->ops->free(sreader->mem, sreader);
  return 0;
}
