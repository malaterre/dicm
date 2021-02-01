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

struct _dicm_options {
    bool stream_filemetaelements;
    bool deflenitem;
    bool deflensq;
    bool group_length;
};

/** stream reader */
struct _dicm_sreader {
  struct _mem *mem;
  struct _src *src;
  struct _dataset dataset;  // current dataset
  struct _filemetaset filemetaset;  // current filemeta

  struct _dicm_options options;
#if 0
  struct _dicm_filepreamble filepreamble;
  struct _dicm_prefix prefix;
  struct _filemetaelement fme;
#endif

  uint32_t curdepos;
  enum state current_state;
};

struct _dicm_sreader *dicm_sreader_init(struct _mem *mem) {
  struct _dicm_sreader *sreader = mem->ops->alloc(mem, sizeof *sreader);
  sreader->mem = mem;
  sreader->options.stream_filemetaelements = false;
  sreader->options.deflenitem = false;
  sreader->options.deflensq = false;
  sreader->options.group_length = false;
  sreader->curdepos = 0;
  sreader->current_state = -1;
  reset_dataset(&sreader->dataset);
  reset_filemetaset(&sreader->filemetaset);
  return sreader;
}

void dicm_sreader_set_src(struct _dicm_sreader *sreader, struct _src *src) {
  sreader->src = src;
}

void dicm_sreader_stream_filemetaelements(struct _dicm_sreader *sreader, bool stream_filemetaelements)
{
  sreader->options.stream_filemetaelements = stream_filemetaelements;
}

void dicm_sreader_group_length(struct _dicm_sreader *sreader, bool group_length)
{
  sreader->options.group_length = group_length;
}

size_t dicm_sreader_pull_filemetaelement_value(
    struct _dicm_sreader *sreader, const struct _filemetaelement *fme,
    char *buf, size_t buflen) {
  assert(fme->vl != kUndefinedLength);
  struct _src *src = sreader->src;
  size_t remaining_len = fme->vl;
  size_t len = buflen < remaining_len ? buflen : remaining_len;
  if (buf) {
    size_t readlen = src->ops->read(src, buf, len);
    assert(readlen == len);  // TODO
    sreader->curdepos += len;
    return readlen;
  } else {
    src->ops->seek(src, len);
    sreader->curdepos += len;
    return len;
  }
}

static inline uint32_t get_filemetaeelement_length(const struct _filemetaelement *fme) {
  assert(fme->vl != kUndefinedLength);
  if (is_vr16(fme->vr)) {
    return 4 /* tag */ + 4 /* VR/VL */ + fme->vl /* VL */;
  }
  return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + fme->vl /* VL */;
}

bool dicm_sreader_read_meta_info(struct _dicm_sreader *sreader) {
  struct _dicm_filepreamble filepreamble;
  struct _dicm_prefix prefix;
  struct _filemetaelement fme;
  struct _src *src = sreader->src;
//  struct _dataset *ds = &sreader->dataset;
  struct _filemetaset *fms = &sreader->filemetaset;

  int next = read_filepreamble(src, fms);
  //  if (!dicm_sreader_hasnext(sreader)) return false;
  //  int next = dicm_sreader_next(sreader);
  sreader->current_state = next;
  if (next == kFilePreamble)
    dicm_sreader_get_file_preamble(sreader, &filepreamble);

#if 0
  if (!dicm_sreader_hasnext(sreader)) return false;
  next = dicm_sreader_next(sreader);
#endif
  next = read_prefix(src, fms);
  sreader->current_state = next;
  assert(next == kDICOMPrefix);
  if (next == kDICOMPrefix)
    dicm_sreader_get_prefix(sreader, &prefix);

  next = read_fme(src, fms);
#if 0
  if (!dicm_sreader_hasnext(sreader)) return false;
  next = dicm_sreader_next(sreader);
#endif
  sreader->current_state = next;
  assert(next == kFileMetaInformationGroupLength);
  if (next == kFileMetaInformationGroupLength)
   dicm_sreader_get_filemetaelement(sreader, &fme);

  union {
    uint32_t ul;
    char bytes[4];
  } group_length;
  assert(fme.vl == sizeof group_length);
#if 0
  dicm_sreader_pull_filemetaelement_value(sreader, &fme, group_length.bytes,
                                          sizeof group_length);
#else
  group_length.ul = fms->fmelen;
#endif

  uint32_t gl = 0;
  char buf[64];
  while (gl != group_length.ul) {
    next = read_fme(src, fms);
    sreader->current_state = next;
    if (next != kFileMetaElement) {
      assert(0);
    }
    if (!dicm_sreader_get_filemetaelement(sreader, &fme)) {
      assert(0);
    }
    if (dicm_sreader_pull_filemetaelement_value(sreader, &fme, buf,
                                                sizeof buf) != fme.vl) {
      assert(0);
    }
    gl += get_filemetaeelement_length(&fme);
    assert(gl <= group_length.ul);
  }
  assert(gl == group_length.ul);

  sreader->current_state = kEndFileMetaInformation;
  sreader->curdepos = 0;
  return true;
}

int dicm_read_explicit(struct _src *src, struct _dataset *ds, const struct _dicm_options *options) {
  const bool deflenitem = options->deflenitem;
  const bool deflensq = options->deflensq;
  const bool group_length = options->group_length;
  // For defined length Item and Defined length SQ we need to create synthetic
  // Delimitation Item. Handle those pseudo event here:
  if (get_deflenitem(ds) == get_curdeflenitem(ds)) {
    // End of Defined Length Item
    reset_cur_defined_length_item(ds);
    return kItemDelimitationItem;
  } else if (get_deflensq(ds) == get_curdeflensq(ds)) {
    // End of Defined Length Sequence
    reset_cur_defined_length_sequence(ds);
    return kSequenceOfItemsDelimitationItem;
  } else if (group_length && ds->grouplen == ds->curgrouplen) {
    ds->curgroup = 0; // reset
    ds->grouplen = kUndefinedLength;
    ds->curgrouplen = 0;

    return kEndGroupDataElement;
  }
  const enum state s = read_explicit_impl(src, ds);

  return s;
}


static int dicm_sreader_hasnext_impl(struct _dicm_sreader *sreader) {
  struct _src *src = sreader->src;
  const int current_state = sreader->current_state;
  const bool stream_filemetaelements = sreader->options.stream_filemetaelements;
  const bool group_length = sreader->options.group_length;
  const struct _dicm_options *options = &sreader->options;
  struct _dataset *ds = &sreader->dataset;

  // make sure to flush remaining bits from a dataelement
  if (current_state == kBasicOffsetTable || current_state == kFragment ||
      current_state == kDataElement) {
    struct _dataelement de;
    buf_into_dataelement(&sreader->dataset, current_state, &de);

    if (current_state == kDataElement) {
      assert(de.vl != kUndefinedLength);
      if (get_deflenitem(ds) != kUndefinedLength) {
        // are we processing a defined length Item ?
        set_curdeflenitem(ds, get_curdeflenitem(ds) + compute_len(&de));
      }
      if (get_deflensq(ds) != kUndefinedLength) {
        // are we processing a defined length SQ ?
        set_curdeflensq(ds, get_curdeflensq(ds) + compute_len(&de));
      }
    }

    const uint_fast16_t element = get_element(de.tag);
    if (element == 0x0) {
      if (group_length) {
        union {
          uint32_t ul;
          char bytes[4];
        } group_length;
        assert(de.vl == 4);
        dicm_sreader_pull_dataelement_value(sreader, &de, group_length.bytes,
                                            de.vl);
        assert(sreader->curdepos == de.vl);
        sreader->curdepos = 0;

        struct _dataset *ds = &sreader->dataset;
        assert(ds->curgroup == 0);
        ds->curgroup = get_group(de.tag);
        assert(ds->grouplen == kUndefinedLength);
        ds->grouplen = group_length.ul;
        // group length include all but the group length element
        assert(ds->curgrouplen == 0);

        sreader->current_state = kGroupLengthDataElement;
        return sreader->current_state;
      } else {
        // FIXME swallow group length
        dicm_sreader_pull_dataelement_value(sreader, &de, NULL, de.vl);
        assert(sreader->curdepos == de.vl);
        sreader->curdepos = 0;
      }
    } else {
      if (group_length) {
        struct _dataset *ds = &sreader->dataset;
        if (ds->curgroup == get_group(de.tag)) {
          ds->curgrouplen += compute_len(&de);
          assert(ds->curgrouplen <= ds->grouplen);
        }
      }
      // not a group length data element:
      assert(element != 0x0);
      dicm_sreader_pull_dataelement_value(sreader, &de, NULL, de.vl);
      assert(sreader->curdepos == de.vl);
      sreader->curdepos = 0;
    }
  } else if (current_state == kFileMetaElement) {
    assert(stream_filemetaelements);
    struct _filemetaelement de;
    buf_into_filemetaelement(&sreader->filemetaset, current_state, &de );
    assert(sreader->curdepos == 0);
    dicm_sreader_pull_filemetaelement_value(sreader, &de, NULL, de.vl);
    assert(sreader->curdepos == de.vl);
    sreader->curdepos = 0;
  }

  struct _filemetaset *fms = &sreader->filemetaset;

  assert(!src->ops->at_end(src));
  switch (current_state) {
    case -1:
      sreader->current_state = kStartFileMetaInformation;
      break;

    case kStartFileMetaInformation:
      if (stream_filemetaelements)
        sreader->current_state = read_filepreamble(src, fms);
      else {
        dicm_sreader_read_meta_info(sreader);
        assert(sreader->current_state == kEndFileMetaInformation);
      }
      break;

    case kFilePreamble:
      assert(stream_filemetaelements);
      sreader->current_state = read_prefix(src, fms);
      break;

    case kDICOMPrefix:
      assert(stream_filemetaelements);
      sreader->current_state = read_fme(src, fms);
      assert(sreader->current_state == kFileMetaInformationGroupLength);
      break;

    case kFileMetaInformationGroupLength:
      assert(stream_filemetaelements);
      sreader->current_state = read_fme(src, fms);
      break;

    case kFileMetaElement:
      assert(stream_filemetaelements);
      sreader->current_state = read_fme(src, fms);
      break;

    case kEndFileMetaInformation:
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kDataElement:
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kGroupLengthDataElement:
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kEndGroupDataElement:
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kItem:
      // de->tag = 0;  // FIXME tag ordering handling
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kBasicOffsetTable:
      // de->tag = 0;
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kFragment:
      // de->tag = 0;
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kItemDelimitationItem:
      // de->tag = 0;
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kSequenceOfItemsDelimitationItem:
      // de->tag = 0;
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kSequenceOfFragmentsDelimitationItem:
      // de->tag = 0;
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kSequenceOfItems:
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    case kSequenceOfFragments:
      sreader->current_state = dicm_read_explicit(src, ds, options);
      break;

    default:
      assert(0);  // Programmer error
  }
  return sreader->current_state;
}

bool dicm_sreader_hasnext(struct _dicm_sreader *sreader) {
  struct _src *src = sreader->src;
  int ret = dicm_sreader_hasnext_impl(sreader);
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
  assert(sreader->filemetaset.bufsize == 128);
  memcpy(fp->data, sreader->filemetaset.buffer, sizeof fp->data);
  return true;
}

bool dicm_sreader_get_prefix(struct _dicm_sreader *sreader,
                             struct _dicm_prefix *prefix) {
  if (sreader->current_state != kDICOMPrefix) return false;
  assert(sreader->filemetaset.bufsize == 4);
  memcpy(prefix->data, sreader->filemetaset.buffer, sizeof prefix->data);
  return true;
}

bool dicm_sreader_get_dataelement(struct _dicm_sreader *sreader,
                                  struct _dataelement *de) {
  // FIXME would be nice to setup an error handler here instead of returning
  // NULL
  const bool stream_filemetaelements = sreader->options.stream_filemetaelements;
  if (sreader->current_state != kDataElement &&
      (stream_filemetaelements && sreader->current_state != kFileMetaElement) &&
//      (stream_filemetaelements && sreader->current_state != kFileMetaInformationGroupLength) &&
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
  if (sreader->current_state != kFileMetaElement &&
      sreader->current_state != kFileMetaInformationGroupLength)
    return false;
  buf_into_filemetaelement(&sreader->filemetaset, sreader->current_state, fme);
  return true;
}

int dicm_sreader_fini(struct _dicm_sreader *sreader) {
  sreader->mem->ops->free(sreader->mem, sreader);
  return 0;
}

size_t dicm_sreader_pull_dataelement_value(struct _dicm_sreader *sreader,
                                           const struct _dataelement *de,
                                           char *buf, size_t buflen) {
  if (de->vl == kUndefinedLength) return kUndefinedLength;
  struct _src *src = sreader->src;
  size_t remaining_len = de->vl - sreader->curdepos;
  size_t len = buflen < remaining_len ? buflen : remaining_len;
  if (buf) {
    size_t readlen = src->ops->read(src, buf, len);
    assert(readlen == len);  // TODO
    sreader->curdepos += readlen;
    return readlen;
  } else {
    src->ops->seek(src, len);
    sreader->curdepos += len;
    return len;
  }
}

struct _dataset *dicm_sreader_get_dataset(struct _dicm_sreader *sreader) {
  return &sreader->dataset;
}
