#include "parser.h"

#include "dicm-private.h"
#include "dicm.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static inline bool isvr_valid(const uvr_t uvr) {
  if (uvr.str[0] < 'A' || uvr.str[0] > 'Z' || /* uppercase A-Z only */
      uvr.str[1] < 'A' || uvr.str[1] > 'Z')
    return false;
  return true;
}

static inline bool tag_is_equal(const struct _dataelement *de, tag_t tag) {
  return de->tag == tag;
}

static inline bool tag_is_lower(const struct _dataelement *de, tag_t tag) {
  return de->tag < tag;
}

static inline bool is_start(const struct _dataelement *de) {
  static const tag_t start = MAKE_TAG(0xfffe, 0xe000);
  return de->tag == start;
}
static inline bool is_end_item(const struct _dataelement *de) {
  static const tag_t end_item = MAKE_TAG(0xfffe, 0xe00d);
  return de->tag == end_item;
}
static inline bool is_end_sq(const struct _dataelement *de) {
  static const tag_t end_sq = MAKE_TAG(0xfffe, 0xe0dd);
  return de->tag == end_sq;
}
static inline bool is_encapsulated_pixel_data(const struct _dataelement *de) {
  static const tag_t pixel_data = MAKE_TAG(0x7fe0, 0x0010);
  const bool is_pixel_data = tag_is_equal(de, pixel_data);
  if (is_pixel_data) {
    // Make sure Pixel Data is Encapsulated (Sequence of Fragments):
    if (de->vl == (uint32_t)-1 && (de->vr == kOB || de->vr == kOW)) {
      return true;
    }
  }
  return false;
}
static inline bool is_undef_len(const struct _dataelement *de) {
  const bool b = de->vl == (uint32_t)-1;
  if (b) {
    return de->vr == kSQ || is_encapsulated_pixel_data(de) || is_start(de);
  }
  return b;
}
static inline uint32_t compute_len(const struct _dataelement *de) {
  assert(!is_undef_len(de));
  if (isvr32(de->vr)) {
    return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + de->vl /* VL */;
  }
  return 4 /* tag */ + 4 /* VR/VL */ + de->vl /* VL */;
}
static inline uint32_t compute_undef_len(const struct _dataelement *de,
                                         uint32_t len) {
  assert(is_undef_len(de));
  assert(len != (uint32_t)-1);
  return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + len;
}

static int read_explicit1(struct _dataelement *de, const char *buf, size_t len) {
  utag_t t;
  uvr_t vr;

  assert(len == 6);
  // Tag
  // size_t n = fread( t.tags, sizeof *t.tags, 2, stream );
  memcpy(t.tags, buf, sizeof *t.tags * 2);
  // if( n != 4 ) return false;
  SWAP_TAG(t);
  if (!tag_is_lower(de, t.tag)) return kOutOfOrder;

  // Value Representation
  // n = fread( vr.str, sizeof *vr.str, 2, stream );
  memcpy(vr.str, buf + 4, sizeof *vr.str * 2);
  /* a lot of VR are not valid (eg: non-ASCII), however the standard may add
   * them in a future edition, so only exclude the impossible ones */
  if (/*n != 2 ||*/ !isvr_valid(vr)) return kInvalidVR;

  de->tag = t.tag;
  de->vr = vr.vr;
  return 0;
}

static int read_explicit2(struct _dataelement *de, const char *buf, size_t len) {
  uvl_t vl;

  // padding and/or 16bits VL
  uvl16_t vl16;
  // n = fread( vl16.bytes, sizeof *vl16.bytes, 2, stream );
  memcpy(vl16.bytes, buf + 0 + 0, sizeof *vl16.bytes * 2);
  // if( n != 2 ) return false;

  // Value Length
  if (isvr32(de->vr)) {
    assert(len == 6);
    /* padding must be set to zero */
    if (vl16.vl16 != 0) return false;

    // n = fread( vl.bytes, 1, 4, stream );
    memcpy(vl.bytes, buf + 0 + 0 + 2, 1 * 4);
    // if( n != 4 ) return false;
    SWAP_VL(vl.vl);
  } else {
    assert(len == 2);
    SWAP_VL16(vl16.vl16);
    vl.vl = vl16.vl16;
  }
  de->vl = vl.vl;
  return true;
}

int read_explicit(struct _src *src, struct _dataelement *de) {
  char buf[16];
  size_t ret =    src->ops->read(src, buf, 4 + 2);
  if( ret == (size_t)-1 ) return ret;
      read_explicit1(de, buf, 4 + 2);
      {
        size_t llen = get_explicit2_len(de);
        src->ops->read(src, buf, llen);
        read_explicit2(de, buf, llen);
      }
      src->ops->seek(src, de->vl);
}

void print_dataelement(struct _dataelement *de) {
  printf("%04x,%04x %.2s %d\n", (unsigned int)get_group(de->tag),
         (unsigned int)get_element(de->tag), get_vr(de->vr).str
         , de->vl);
}
