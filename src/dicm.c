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

#include "dicm-parser.h"
#include "dicm-private.h"

#include <assert.h> /* assert */
#include <stdlib.h> /* malloc/free */
#include <string.h> /* memset */

/** stream reader */
struct _dicm_sreader {
  struct _mem *mem;
  struct _src *src;
  //struct _dataelement dataelement;  // current dataelement
  struct _dataset dataset;  // current dataset
  enum state current_state;
};

struct _dicm_sreader *dicm_sreader_init(struct _mem *mem, struct _src *src) {
  struct _dicm_sreader *sreader = mem->ops->alloc(mem, sizeof *sreader);
  sreader->mem = mem;
  sreader->src = src;
  sreader->current_state = kStartInstance;
  memset(sreader->dataset.buffer, 0, sizeof sreader->dataset.buffer);
  sreader->dataset.bufsize = 0;  // sizeof sreader->buffer;
  reset_dataset(&sreader->dataset);
  return sreader;
}

static int dicm_sreader_impl(struct _dicm_sreader *sreader) {
  struct _src *src = sreader->src;
  int current_state = sreader->current_state;
  struct _dataset *ds = &sreader->dataset;
  struct _dataelement *de = &ds->de;

  if (src->ops->at_end(src)) {
    // dead code ??
    sreader->current_state = kEndInstance;
    assert(0);
    return sreader->current_state;
  }
  switch (current_state) {
    case kStartInstance:
      sreader->current_state = read_filepreamble(src, ds);
      break;

    case kFilePreamble:
      sreader->current_state = read_prefix(src, ds);
      break;

    case kPrefix:
      sreader->current_state = read_explicit(src, ds);
      break;

    case kFileMetaElement:
      sreader->current_state = read_explicit(src, ds);
      break;

    case kDataElement:
      sreader->current_state = read_explicit(src, ds);
      break;

    case kItem:
      de->tag = 0; // FIXME tag ordering handling
      sreader->current_state = read_explicit(src, ds);
      break;

    case kBasicOffsetTable:
      de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;

    case kFragment:
      de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;


    case kItemDelimitationItem:
      de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;

    case kSequenceOfItemsDelimitationItem:
      de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;

    case kSequenceOfFragmentsDelimitationItem:
      de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;

    case kSequenceOfItems:
      sreader->current_state = read_explicit(src, ds);
      break;

    case kSequenceOfFragments:
      sreader->current_state = read_explicit(src, ds);
      break;

    case kEndInstance:
      /* Do something different and set current_state */
      break;

    default:
      assert(0);  // Programmer error
  }
  return sreader->current_state;
}

int dicm_sreader_hasnext(struct _dicm_sreader *sreader) {
  int ret = dicm_sreader_impl(sreader);
  struct _src *src = sreader->src;
  //printf("ret %d\n", ret);
  // return sreader->current_state != kEndInstance;
  return !src->ops->at_end(src);
}

int dicm_sreader_next(struct _dicm_sreader *sreader) {
  return sreader->current_state;
}

const char *dicm_sreader_get_file_preamble(struct _dicm_sreader *sreader) {
  if (sreader->current_state != kFilePreamble) return NULL;
  return sreader->dataset.buffer;
}

const char *dicm_sreader_get_prefix(struct _dicm_sreader *sreader) {
  if (sreader->current_state != kPrefix) return NULL;
  return sreader->dataset.buffer;
}

struct _dataelement *dicm_sreader_get_dataelement(
    struct _dicm_sreader *sreader) {
  // FIXME would be nice to setup an error handler here instead of returning NULL
  if (sreader->current_state != kDataElement &&
      sreader->current_state != kSequenceOfItems &&
      sreader->current_state != kSequenceOfFragments)
    return NULL;
  return &sreader->dataset.de;
}

struct _filemetaelement *dicm_sreader_get_filemetaelement(
    struct _dicm_sreader *sreader) {
  if (sreader->current_state != kFileMetaElement) return NULL;
  return (struct _filemetaelement *)&sreader->dataset.de;  // FIXME
}

int dicm_sreader_fini(struct _dicm_sreader *sreader) {
  sreader->mem->ops->free(sreader->mem, sreader);
  return 0;
}
