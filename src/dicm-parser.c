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

#include "dicm-parser.h"

#include "dicm-private.h"
#include "dicm.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

bool dicm_de_is_start(const struct _dataelement *de) {
  return de->tag == (tag_t)kStart;
}

bool dicm_de_is_end_item(const struct _dataelement *de) {
  return de->tag == (tag_t)kEndItem;
}

bool dicm_de_is_end_sq(const struct _dataelement *de) {
  return de->tag == (tag_t)kEndSQ;
}

bool dicm_de_is_encapsulated_pixel_data(const struct _dataelement *de) {
  return is_encapsulated_pixel_data(de);
}

bool dicm_de_is_sq(const struct _dataelement *de) {
  if (de->vl == (uint32_t)-1 && de->vr == kSQ) {
    // undef sq
    return true;
  } else if (de->vr == kSQ) {
    // defined len sq
    return true;
  }
  return false;
}

int read_filepreamble(struct _src *src, struct _dataset *ds) {
  char *buf = ds->buffer;
  const size_t size = src->ops->read(src, buf, 128);
  if (unlikely(size < 128)) {
    ds->bufsize = size;
    return -kNotEnoughData;
  }
  ds->bufsize = 128;
  return kFilePreamble;
}

int read_prefix(struct _src *src, struct _dataset *ds) {
  char *buf = ds->buffer;
  const size_t size = src->ops->read(src, buf, 4);
  if (unlikely(size < 4)) {
    ds->bufsize = size;
    return -kNotEnoughData;
  }
  ds->bufsize = 4;
  return kPrefix;
}

int buf_into_dataelement(const struct _dataset *ds, enum state current_state,
                         struct _dataelement *de) {
  const char *buf = ds->buffer;
  const size_t bufsize = ds->bufsize;
  union {
    byte_t bytes[12];
    ede32_t ede32;  // explicit data element, VR 32. 12 bytes
    ede16_t ede16;  // explicit data element, VR 16. 8 bytes
    ide_t ide;      // implicit data element, no VR. 8 bytes
  } ude;
  memcpy(ude.bytes, buf, bufsize);
  SWAP_TAG(ude.ide.utag);

  if (current_state == kFileMetaElement || current_state == kDataElement) {
    if (bufsize == 12) {
      de->tag = ude.ede32.utag.tag;
      de->vr = ude.ede32.uvr.vr.vr;
      de->vl = ude.ede32.uvl.vl;
    } else if (bufsize == 8) {
      de->tag = ude.ede16.utag.tag;
      de->vr = ude.ede16.uvr.vr;
      de->vl = ude.ede16.uvl.vl16;
    } else {
      assert(0);
    }
  } else {
    assert(current_state == kSequenceOfItems ||
           current_state == kSequenceOfFragments);
    de->tag = ude.ide.utag.tag;
    de->vr = kINVALID;
    de->vl = ude.ide.uvl.vl;
  }
  return 0;
}
