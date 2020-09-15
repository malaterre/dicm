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

#include "dicm-parser.h"

#include "dicm-private.h"
#include "dicm.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int read_explicit(struct _src *src, struct _dataset *ds) {
  // http://dicom.nema.org/medical/dicom/current/output/chtml/part05/chapter_7.html#sect_7.1.2
  typedef union {
    byte_t bytes[12];
    ede32_t ede32;      // explicit data element. 12 bytes
    ede16_t ede16;  // explicit data element, VR 16. 8 bytes
    ide_t ide;      // implicit data element. 8 bytes
  } ude_t;
  assert(sizeof(ude_t) == 12);
  struct _dataelement *curde = &ds->de;
  tag_t prevtag = curde->tag;

  if (ds->deflenitem == ds->curdeflenitem) {
    // End of Item
    reset_defined_length_item(ds);
    return kItemDelimitationItem;
  } else if (ds->deflensq == ds->curdeflensq) {
    // End of Sequence
    reset_defined_length_sequence(ds);
    return kSequenceOfItemsDelimitationItem;
  }
  const int sequenceoffragments = ds->sequenceoffragments;

  ude_t ude;
  size_t ret = src->ops->read(src, ude.bytes, 8);
  if (unlikely(ret < 8)) {
    return -kNotEnoughData;
  }
#ifdef DOSWAP
  SWAP_TAG(ude.ede32.utag);
#endif

  if (unlikely(is_tag_start(ude.ide.utag.tag))) {
    curde->tag = ude.ide.utag.tag;
    curde->vr = kINVALID;
    curde->vl = ude.ide.uvl.vl;

    if (sequenceoffragments >= 0) {
      src->ops->seek(src, curde->vl);
      ds->sequenceoffragments++;
      return sequenceoffragments == 0 ? kBasicOffsetTable : kFragment;
    } else if (curde->vl != kUndefinedLength) {
      assert(ds->deflenitem == kUndefinedLength);
      assert(curde->vl % 2 == 0);
      ds->deflenitem = ude.ide.uvl.vl;
      if (ds->deflensq != kUndefinedLength) {
        // are we processing a defined length SQ ?
        ds->curdeflensq += 4 + 4;
      }
    }

    return kItem;
  } else if (unlikely(is_tag_end_item(ude.ide.utag.tag)) ||
             unlikely(is_tag_end_sq(ude.ide.utag.tag))) {
    curde->tag = ude.ide.utag.tag;
    curde->vr = kINVALID;
    curde->vl = ude.ide.uvl.vl;

    if (unlikely(ude.ide.uvl.vl != 0)) return -kDicmReservedNotZero;

    if (sequenceoffragments >= 0) {
      ds->sequenceoffragments = -1;
      return kSequenceOfFragmentsDelimitationItem;
    }
    // return ude.ide.utag.tag == (tag_t)kEndItem
    return is_tag_end_item(ude.ide.utag.tag) ? kItemDelimitationItem
                                             : kSequenceOfItemsDelimitationItem;
  }

  if (unlikely(!tag_is_lower(curde, ude.ide.utag.tag))) {
    return -kDicmOutOfOrder;
  }

  // VR16 ?
  dataelement_t de;
  if (is_vr16(ude.ede16.uvr.vr)) {
    de.tag = ude.ede16.utag.tag;
    de.vr = ude.ede16.uvr.vr;
    de.vl = ude.ede16.uvl.vl16;
  } else {
    // padding must be set to zero
    if (unlikely(ude.ede32.uvr.vr.reserved != 0)) return -kDicmReservedNotZero;

    ret = src->ops->read(src, ude.ede32.uvl.bytes, 4);
    if (unlikely(ret < 4)) return -kNotEnoughData;

    de.tag = ude.ede32.utag.tag;
    de.vr = ude.ede32.uvr.vr.vr;
    de.vl = ude.ede32.uvl.vl;
  }

  if(de.vl != kUndefinedLength && de.vl % 2 != 0) return -kDicmOddDefinedLength;
  curde->tag = de.tag;
  curde->vr = de.vr;
  curde->vl = de.vl;

  if (tag_get_group(ude.ede32.utag.tag) == 0x2) {
    assert(de.vl != kUndefinedLength);
    src->ops->seek(src, curde->vl);
    return kFileMetaElement;
  } else if (is_tag_pixeldata(ude.ede32.utag.tag) && ude.ede32.uvr.vr.vr == kOB &&
             ude.ede32.uvl.vl == kUndefinedLength) {
    assert(sequenceoffragments == -1);
    ds->sequenceoffragments = 0;
    return kSequenceOfFragments;
  } else if (ude.ede32.uvr.vr.vr == kSQ && ude.ede32.uvl.vl == kUndefinedLength) {
    return kSequenceOfItems;
  } else if (ude.ede32.uvr.vr.vr == kSQ) {
    // defined length SQ
    assert(ds->deflensq == kUndefinedLength);
    ds->deflensq = ude.ede32.uvl.vl;
    return kSequenceOfItems;
  } else if (likely(tag_get_group(ude.ede32.utag.tag) >= 0x8)) {
    assert(de.vl != kUndefinedLength);
    src->ops->seek(src, curde->vl);
    if (ds->deflenitem != kUndefinedLength) {
      // are we processing a defined length Item ?
      ds->curdeflenitem += compute_len(curde);
    }
    if (ds->deflensq != kUndefinedLength) {
      // are we processing a defined length SQ ?
      ds->curdeflensq += compute_len(curde);
    }
    return kDataElement;
  }
  assert(0);
  return -kInvalidTag;
}
