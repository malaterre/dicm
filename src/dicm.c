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
  char buffer[128 /*4096*/];  // Minimal amount of memory (preamble is the
                              // bigest one ?)
  size_t bufsize;             //
};

struct _dicm_sreader *dicm_sreader_init(struct _mem *mem, struct _src *src) {
  struct _dicm_sreader *sreader = mem->ops->alloc(mem, sizeof *sreader);
  sreader->mem = mem;
  sreader->src = src;
  sreader->current_state = kStartInstance;
  memset(sreader->buffer, 0, sizeof sreader->buffer);
  sreader->bufsize = 0;  // sizeof sreader->buffer;
  sreader->dataset.de.tag = 0;
  sreader->dataset.deflensq = kUndefinedLength;
  sreader->dataset.curdeflensq = 0;
  sreader->dataset.deflenitem = kUndefinedLength;
  sreader->dataset.curdeflenitem = 0;
  sreader->dataset.sequenceoffragments = -1;
  return sreader;
}

static int dicm_sreader_impl(struct _dicm_sreader *sreader) {
  struct _src *src = sreader->src;
  char *buf = sreader->buffer;
  int current_state = sreader->current_state;
  int ret;
  struct _dataset *ds = &sreader->dataset;
  struct _dataelement *de = &ds->de;

  if (src->ops->at_end(src)) {
    // dead code ??
    sreader->current_state = kEndInstance;
    assert(0);
    return sreader->current_state;
  }
  size_t bufsize;
  switch (current_state) {
    case kStartInstance:
      /* Do something with input and set current_state */
      // get new input:
      bufsize = 128;
      sreader->bufsize = src->ops->read(src, buf, bufsize);
      sreader->current_state = kFilePreamble;
      break;

    case kFilePreamble:
      bufsize = 4;
      sreader->bufsize = src->ops->read(src, buf, bufsize);
      sreader->current_state = kPrefix;
      break;

    case kPrefix:
      ret = read_explicit(src, ds);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        if (dicm_de_get_group(de) == 0x2)
          sreader->current_state = kFileMetaElement;
        else
          assert(0);
      }
      assert(ret == sreader->current_state);
      break;

    case kFileMetaElement:
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        if (dicm_de_get_group(de) == 0x2) {
          sreader->current_state = kFileMetaElement;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else
          assert(0);
      }
      assert(ret == sreader->current_state);
      break;

    case kDataElement:
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        if (dicm_de_is_start(de)) {
          sreader->current_state = kItem;
        } else if (dicm_de_is_end_item(de)) {
          sreader->current_state = kItemDelimitationItem;
        } else if (dicm_de_is_end_sq(de)) {
          sreader->current_state = kSequenceOfItemsDelimitationItem;
        } else if (dicm_de_is_encapsulated_pixel_data(de)) {
          sreader->current_state = kSequenceOfFragments;
        } else if (dicm_de_is_sq(de)) {
          sreader->current_state = kSequenceOfItems;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else {
          assert(0);
        }
      }
      sreader->current_state = ret;
      assert(ret == sreader->current_state);
      break;

    case kItem:
      de->tag = 0;
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        // memcpy(&sreader->dataelement, de, sizeof *de);
        if (dicm_de_is_start(de))
          sreader->current_state = kItem;
        else if (dicm_de_is_end_item(de)) {
          sreader->current_state = kItemDelimitationItem;
        } else if (dicm_de_is_end_sq(de)) {
          sreader->current_state = kSequenceOfItemsDelimitationItem;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else {
          assert(0);
        }
      }
      sreader->current_state = ret;
      assert(ret == sreader->current_state);
      break;

    case kBasicOffsetTable:
      de->tag = 0;
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        // memcpy(&sreader->dataelement, de, sizeof *de);
        if (dicm_de_is_start(de))
          sreader->current_state = kItem;
        else if (dicm_de_is_end_item(de)) {
          sreader->current_state = kItemDelimitationItem;
        } else if (dicm_de_is_end_sq(de)) {
          sreader->current_state = kSequenceOfItemsDelimitationItem;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else {
          assert(0);
        }
      }
      sreader->current_state = ret;
      assert(ret == sreader->current_state);
      break;

    case kFragment:
      de->tag = 0;
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        // memcpy(&sreader->dataelement, de, sizeof *de);
        if (dicm_de_is_start(de))
          sreader->current_state = kItem;
        else if (dicm_de_is_end_item(de)) {
          sreader->current_state = kItemDelimitationItem;
        } else if (dicm_de_is_end_sq(de)) {
          sreader->current_state = kSequenceOfItemsDelimitationItem;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else {
          assert(0);
        }
      }
      sreader->current_state = ret;
      assert(ret == sreader->current_state);
      break;


    case kItemDelimitationItem:
      de->tag = 0;
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        if (dicm_de_is_start(de)) {
          sreader->current_state = kItem;
        } else if (dicm_de_is_end_item(de)) {
          sreader->current_state = kItemDelimitationItem;
        } else if (dicm_de_is_end_sq(de)) {
          sreader->current_state = kSequenceOfItemsDelimitationItem;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else {
          //assert(0);
        }
      }
      sreader->current_state = ret;
      assert(ret == sreader->current_state);
      break;

    case kSequenceOfItemsDelimitationItem:
      de->tag = 0;
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        if (dicm_de_is_start(de)) {
          sreader->current_state = kItem;
        } else if (dicm_de_is_end_item(de)) {
          sreader->current_state = kItemDelimitationItem;
        } else if (dicm_de_is_end_sq(de)) {
          sreader->current_state = kSequenceOfItemsDelimitationItem;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else {
          assert(0);
        }
      }
      assert(ret == sreader->current_state);
      break;

    case kSequenceOfFragmentsDelimitationItem:
      de->tag = 0;
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        if (dicm_de_is_start(de)) {
          sreader->current_state = kItem;
        } else if (dicm_de_is_end_item(de)) {
          sreader->current_state = kItemDelimitationItem;
        } else if (dicm_de_is_end_sq(de)) {
          sreader->current_state = kSequenceOfItemsDelimitationItem;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else {
          assert(0);
        }
      }
      sreader->current_state = ret;
      assert(ret == sreader->current_state);
      break;

    case kSequenceOfItems:
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        // memcpy(&sreader->dataelement, de, sizeof *de);
        if (dicm_de_is_start(de))
          sreader->current_state = kItem;
        else if (dicm_de_is_end_item(de)) {
          sreader->current_state = kItemDelimitationItem;
        } else if (dicm_de_is_end_sq(de)) {
          sreader->current_state = kSequenceOfItemsDelimitationItem;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else {
          assert(0);
        }
      }
      sreader->current_state = ret;
      assert(ret == sreader->current_state);
      break;

    case kSequenceOfFragments:
      ret = read_explicit(src, de);
      if (ret < 0) {
        sreader->current_state = ret;  // kEndInstance;
      } else {
        // memcpy(&sreader->dataelement, de, sizeof *de);
        if (dicm_de_is_start(de))
          sreader->current_state = kItem;
        else if (dicm_de_is_end_item(de)) {
          sreader->current_state = kItemDelimitationItem;
        } else if (dicm_de_is_end_sq(de)) {
          sreader->current_state = kSequenceOfItemsDelimitationItem;
        } else if (dicm_de_get_group(de) >= 0x8) {
          // memcpy(&sreader->dataelement, de, sizeof *de);
          sreader->current_state = kDataElement;
        } else {
          assert(0);
        }
      }
      sreader->current_state = ret;
      assert(ret == sreader->current_state);
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
  // printf("ret %d\n", ret);
  // return sreader->current_state != kEndInstance;
  return !src->ops->at_end(src);
}

int dicm_sreader_next(struct _dicm_sreader *sreader) {
  return sreader->current_state;
}

const char *dicm_sreader_get_file_preamble(struct _dicm_sreader *sreader) {
  if (sreader->current_state != kFilePreamble) return NULL;
  return sreader->buffer;
}

const char *dicm_sreader_get_prefix(struct _dicm_sreader *sreader) {
  if (sreader->current_state != kPrefix) return NULL;
  return sreader->buffer;
}

struct _dataelement *dicm_sreader_get_dataelement(
    struct _dicm_sreader *sreader) {
  if (sreader->current_state != kFileMetaElement &&
      sreader->current_state != kDataElement &&
      sreader->current_state != kSequenceOfItems &&
      sreader->current_state != kSequenceOfFragments)
    return NULL;
  return &sreader->dataset.de;
}

int dicm_sreader_fini(struct _dicm_sreader *sreader) {
  sreader->mem->ops->free(sreader->mem, sreader);
  return 0;
}
