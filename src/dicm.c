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
  struct _dataset dataset;  // current dataset
  uint32_t curdepos;
  enum state current_state;
};

struct _dicm_sreader *dicm_sreader_init(struct _mem *mem, struct _src *src) {
  struct _dicm_sreader *sreader = mem->ops->alloc(mem, sizeof *sreader);
  sreader->mem = mem;
  sreader->src = src;
  sreader->curdepos = 0;
  sreader->current_state = -1;  // kStartInstance;
  reset_dataset(&sreader->dataset);
  return sreader;
}

static int dicm_sreader_impl(struct _dicm_sreader *sreader) {
  struct _src *src = sreader->src;
  const int current_state = sreader->current_state;
  // make sure to flush remaining bits from a dataelement
  if (current_state == kBasicOffsetTable || current_state == kFragment ||
      current_state == kFileMetaElement || current_state == kDataElement) {
    struct _dataelement de;
    buf_into_dataelement(&sreader->dataset, current_state, &de);
    dicm_sreader_pull_dataelement_value(sreader, &de, NULL, de.vl);
  }

  struct _dataset *ds = &sreader->dataset;

  assert(!src->ops->at_end(src));
  switch (current_state) {
    case -1:
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
      // de->tag = 0;  // FIXME tag ordering handling
      sreader->current_state = read_explicit(src, ds);
      break;

    case kBasicOffsetTable:
      // de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;

    case kFragment:
      // de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;

    case kItemDelimitationItem:
      // de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;

    case kSequenceOfItemsDelimitationItem:
      // de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;

    case kSequenceOfFragmentsDelimitationItem:
      // de->tag = 0;
      sreader->current_state = read_explicit(src, ds);
      break;

    case kSequenceOfItems:
      sreader->current_state = read_explicit(src, ds);
      break;

    case kSequenceOfFragments:
      sreader->current_state = read_explicit(src, ds);
      break;

    default:
      assert(0);  // Programmer error
  }
  return sreader->current_state;
}

bool dicm_sreader_hasnext(struct _dicm_sreader *sreader) {
  struct _src *src = sreader->src;
  int ret = dicm_sreader_impl(sreader);
  if (ret < 0) {
    assert(src->ops->at_end(src));
  }
  // printf("ret %d\n", ret);
  return !src->ops->at_end(src);
}

int dicm_sreader_next(struct _dicm_sreader *sreader) {
  assert(sreader->current_state != -1);
  return sreader->current_state;
}

bool dicm_sreader_get_file_preamble(struct _dicm_sreader *sreader,
                                    struct _dicm_filepreamble *fp) {
  if (sreader->current_state != kFilePreamble) return false;
  assert(sreader->dataset.bufsize == 128);
  memcpy(fp->data, sreader->dataset.buffer, sizeof fp->data);
  return true;
}

bool dicm_sreader_get_prefix(struct _dicm_sreader *sreader,
                             struct _dicm_prefix *prefix) {
  if (sreader->current_state != kPrefix) return false;
  assert(sreader->dataset.bufsize == 4);
  memcpy(prefix->data, sreader->dataset.buffer, sizeof prefix->data);
  return true;
}

bool dicm_sreader_get_dataelement(struct _dicm_sreader *sreader,
                                  struct _dataelement *de) {
  // FIXME would be nice to setup an error handler here instead of returning
  // NULL
  if (sreader->current_state != kDataElement &&
      sreader->current_state != kSequenceOfItems &&
      sreader->current_state != kSequenceOfFragments &&
      sreader->current_state != kItem &&
      sreader->current_state != kBasicOffsetTable &&
      sreader->current_state != kFragment &&
      sreader->current_state != kItemDelimitationItem &&
      sreader->current_state != kSequenceOfItemsDelimitationItem &&
      sreader->current_state != kSequenceOfFragmentsDelimitationItem)
    return false;
  buf_into_dataelement(&sreader->dataset, sreader->current_state, de);
  return true;
}

bool dicm_sreader_get_filemetaelement(struct _dicm_sreader *sreader,
                                      struct _filemetaelement *fme) {
  if (sreader->current_state != kFileMetaElement) return false;
  buf_into_dataelement(&sreader->dataset, sreader->current_state,
                       (struct _dataelement *)fme);  // FIXME
  return true;
}

int dicm_sreader_fini(struct _dicm_sreader *sreader) {
  sreader->mem->ops->free(sreader->mem, sreader);
  return 0;
}

size_t dicm_sreader_pull_dataelement_value(struct _dicm_sreader *sreader,
                                           const struct _dataelement *de,
                                           char *buf, size_t buflen) {
  struct _src *src = sreader->src;
  size_t remaining_len = de->vl - sreader->curdepos;
  size_t len = buflen < remaining_len ? buflen : remaining_len;
  if (buf) {
    size_t readlen = src->ops->read(src, buf, len);
    assert(readlen == len);  // TODO
    sreader->curdepos += readlen;
    return readlen;
  } else {
    assert(sreader->curdepos == 0);
    assert(de->vl == len);
    src->ops->seek(src, len);
    return len;
  }
}
