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

// objdump -d -M intel -S ./src/CMakeFiles/dicm.dir/dicm-parser-exp.c.o
// unlikely only for error condition (short buffer read)

#include "dicm-parser.h"

#include "dicm-private.h"
#include "dicm.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/**
 * Implementation detail. All the work will simply parse the file structure. No
 * work will be done to byte swap the Data Element Tag or VR
 */
int read_explicit_impl(struct _src *src, struct _dataset *ds) {
  // http://dicom.nema.org/medical/dicom/current/output/chtml/part05/chapter_7.html#sect_7.1.2
  union {
    byte_t bytes[12];
    ede32_t ede32;  // explicit data element, VR 32. 12 bytes
    ede16_t ede16;  // explicit data element, VR 16. 8 bytes
    ide_t ide;      // implicit data element, no VR. 8 bytes
  } ude;
  assert(sizeof(ude) == 12);
  char *buf = ds->buffer;

  const int sequenceoffragments = ds->sequenceoffragments;

  size_t ret = src->ops->read(src, ude.bytes, 8);
  if (unlikely(ret < 8)) return -kNotEnoughData;

  if (is_tag_start(ude.ide.utag.tag)) {
    memcpy(buf, ude.bytes, sizeof ude.ide);
    ds->bufsize = sizeof ude.ide;

    // 1. Sequence of Fragments
    if (sequenceoffragments >= 0) {
      return sequenceoffragments == 0 ? kBasicOffsetTable : kFragment;
    }

    // or 2. Sequence of Items:
    return kItem;
  } else if (is_tag_end_item(ude.ide.utag.tag)) {
    memcpy(buf, ude.bytes, sizeof ude.ide);
    ds->bufsize = sizeof ude.ide;

    if (unlikely(ude.ide.uvl.vl != 0)) return -kDicmReservedNotZero;

    return kItemDelimitationItem;
  } else if (is_tag_end_sq(ude.ide.utag.tag)) {
    memcpy(buf, ude.bytes, sizeof ude.ide);
    ds->bufsize = sizeof ude.ide;

    if (unlikely(ude.ide.uvl.vl != 0)) return -kDicmReservedNotZero;

    // 1. Sequence of Fragments
    if (sequenceoffragments >= 0) {
      return kSequenceOfFragmentsDelimitationItem;
    }

    // or 2. Sequence of Items:
    return kSequenceOfItemsDelimitationItem;
  }

  // VR16 ?
  if (is_vr16(ude.ede16.uvr.vr)) {
    memcpy(buf, ude.bytes, sizeof ude.ede16);
    ds->bufsize = sizeof ude.ede16;

    return kDataElement;
  }
  // else
  // padding must be set to zero
  if (unlikely(ude.ede32.uvr.vr.reserved != 0)) return -kDicmReservedNotZero;

  ret = src->ops->read(src, ude.ede32.uvl.bytes, 4);
  if (unlikely(ret < 4)) return -kNotEnoughData;

  memcpy(buf, ude.bytes, sizeof ude.ede32);
  ds->bufsize = sizeof ude.ede32;

  if (is_tag_pixeldata(ude.ede32.utag.tag) &&
      ude.ede32.uvl.vl == kUndefinedLength) {
    return kSequenceOfFragments;
  } else if (ude.ede32.uvr.vr.vr == kSQ ||
             ude.ede32.uvl.vl == kUndefinedLength) {
    return kSequenceOfItems;
  }
  // likely(tag_get_group(ude.ede32.utag.tag) >= 0x0008)
  return kDataElement;
}

int read_fme(struct _src *src, struct _filemetaset *ds) {
  // http://dicom.nema.org/medical/dicom/current/output/chtml/part05/chapter_7.html#sect_7.1.2
  union {
    byte_t bytes[12];
    ede32_t ede32;  // explicit data element, VR 32. 12 bytes
    ede16_t ede16;  // explicit data element, VR 16. 8 bytes
    ide_t ide;      // implicit data element, no VR. 8 bytes
  } ude;
  assert(sizeof(ude) == 12);
  char *buf = ds->buffer;

  if (ds->fmelen == ds->curfmelen) {
    ds->fmelen = kUndefinedLength;
    ds->curfmelen = 0;

    return kEndFileMetaInformation;
  }

  size_t ret = src->ops->read(src, ude.bytes, 8);
  if (unlikely(ret < 8)) return -kNotEnoughData;

  if (unlikely(is_tag_start(ude.ide.utag.tag))) {
    assert(0);
    return kItem;
  } else if (unlikely(is_tag_end_item(ude.ide.utag.tag)) ||
             unlikely(is_tag_end_sq(ude.ide.utag.tag))) {
    assert(0);
    return is_tag_end_item(ude.ide.utag.tag) ? kItemDelimitationItem
                                             : kSequenceOfItemsDelimitationItem;
  }

  // VR16 ?
  dataelement_t de;
  if (is_vr16(ude.ede16.uvr.vr)) {
    de.tag = ude.ede16.utag.tag;
    de.vr = ude.ede16.uvr.vr;
    de.vl = ude.ede16.uvl.vl16;

    memcpy(buf, ude.bytes, sizeof ude.ede16);
    ds->bufsize = sizeof ude.ede16;
  } else {
    // padding must be set to zero
    if (unlikely(ude.ede32.uvr.vr.reserved != 0)) return -kDicmReservedNotZero;

    ret = src->ops->read(src, ude.ede32.uvl.bytes, 4);
    if (unlikely(ret < 4)) return -kNotEnoughData;

    de.tag = ude.ede32.utag.tag;
    de.vr = ude.ede32.uvr.vr.vr;
    de.vl = ude.ede32.uvl.vl;

    memcpy(buf, ude.bytes, sizeof ude.ede32);
    ds->bufsize = sizeof ude.ede32;
  }

  assert(de.vl != kUndefinedLength);
  if (de.vl != kUndefinedLength && de.vl % 2 != 0)
    return -kDicmOddDefinedLength;

  if (tag_get_group(ude.ede32.utag.tag) == 0x0002) {
    // FME Group Length is actually read entirely (including value)
    if (tag_get_element(ude.ede32.utag.tag) == 0x0000) {
      union {
        uint32_t ul;
        char bytes[4];
      } group_length;
      src->ops->read(src, group_length.bytes, 4);
      ds->fmelen = group_length.ul;

      return kFileMetaInformationGroupLength;
    }
    ds->curfmelen += compute_len(&de);
    return kFileMetaElement;
  } else if (is_tag_pixeldata(ude.ede32.utag.tag) &&
             ude.ede32.uvr.vr.vr == kOB &&
             ude.ede32.uvl.vl == kUndefinedLength) {
    assert(0);
    return kSequenceOfFragments;
  } else if (ude.ede32.uvr.vr.vr == kSQ &&
             ude.ede32.uvl.vl == kUndefinedLength) {
    assert(0);
    return kSequenceOfItems;
  } else if (ude.ede32.uvr.vr.vr == kSQ) {
    assert(0);

    return kSequenceOfItems;
  } else if (likely(tag_get_group(ude.ede32.utag.tag) >= 0x0008)) {
    assert(0);
    assert(de.vl != kUndefinedLength);

    return kDataElement;
  }
  assert(0);
  return -kInvalidTag;
}
