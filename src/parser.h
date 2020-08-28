#pragma once

#include <stdbool.h> /* bool */
#include <stddef.h>  /* size_t */
#include <stdint.h>  /* uint16_t */

typedef uint32_t tag_t;
typedef uint16_t vr_t;
typedef uint32_t vl_t;

typedef union {
  uint16_t tags[2];
  tag_t tag;
} utag_t;
typedef union {
  char str[2];
  vr_t vr;
} uvr_t;
typedef union {
  char bytes[4];
  vl_t vl;
} uvl_t;
typedef union {
  char bytes[2];
  uint16_t vl16;
} uvl16_t;

static inline uint_fast16_t get_group(tag_t tag) {
  return (uint16_t)(tag >> 16);
}
static inline uint_fast16_t get_element(tag_t tag) {
  return (uint16_t)(tag & (uint16_t)0xffff);
}

static inline uvr_t get_vr(vr_t vr) {
  uvr_t ret;
  ret.vr = vr;
  return ret;
}

#define MAKE_TAG(group, element) (group << 16 | element)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAP_TAG(t) t.tag = MAKE_TAG(t.tags[0], t.tags[1])
#else
#define SWAP_TAG(t)                \
  t.tags[0] = bswap_16(t.tags[0]); \
  t.tags[1] = bswap_16(t.tags[1])
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MAKE_VR(left, right) (right << 8 | left)
#else
#define MAKE_VR(left, right) (left << 8 | right)
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAP_VL(vl)
#define SWAP_VL16(vl)
#else
#define SWAP_VL(vl) vl = bswap_32(vl)
#define SWAP_VL16(vl) vl = bswap_16(vl)
#endif

// Full list of VRs as of DICOM 2017a
enum {
  kINVALID = 0, /* Item, Item Delimitation Item & Sequence Delimitation Item */
  kAE = MAKE_VR('A', 'E'),
  kAS = MAKE_VR('A', 'S'),
  kAT = MAKE_VR('A', 'T'),
  kCS = MAKE_VR('C', 'S'),
  kDA = MAKE_VR('D', 'A'),
  kDS = MAKE_VR('D', 'S'),
  kDT = MAKE_VR('D', 'T'),
  kFL = MAKE_VR('F', 'L'),
  kFD = MAKE_VR('F', 'D'),
  kIS = MAKE_VR('I', 'S'),
  kLO = MAKE_VR('L', 'O'),
  kLT = MAKE_VR('L', 'T'),
  kOB = MAKE_VR('O', 'B'),
  kOD = MAKE_VR('O', 'D'),
  kOF = MAKE_VR('O', 'F'),
  kOL = MAKE_VR('O', 'L'),
  kOW = MAKE_VR('O', 'W'),
  kPN = MAKE_VR('P', 'N'),
  kSH = MAKE_VR('S', 'H'),
  kSL = MAKE_VR('S', 'L'),
  kSQ = MAKE_VR('S', 'Q'),
  kSS = MAKE_VR('S', 'S'),
  kST = MAKE_VR('S', 'T'),
  kTM = MAKE_VR('T', 'M'),
  kUC = MAKE_VR('U', 'C'),
  kUI = MAKE_VR('U', 'I'),
  kUL = MAKE_VR('U', 'L'),
  kUN = MAKE_VR('U', 'N'),
  kUR = MAKE_VR('U', 'R'),
  kUS = MAKE_VR('U', 'S'),
  kUT = MAKE_VR('U', 'T'),
};

static inline bool isvr32(const vr_t vr) {
  switch (vr) {
    case kAE:
    case kAS:
    case kAT:
    case kCS:
    case kDA:
    case kDS:
    case kDT:
    case kFD:
    case kFL:
    case kIS:
    case kLO:
    case kLT:
    case kPN:
    case kSH:
    case kSL:
    case kSS:
    case kST:
    case kTM:
    case kUI:
    case kUL:
    case kUS:
      /* 16bits: */
      return false;
    case kOB:
    case kOD:
    case kOF:
    case kOL:
    case kOW:
    case kSQ:
    case kUC:
    case kUN:
    case kUR:
    case kUT:
      /* 32bits: */
      return true;
    default:
      /* parser error, or newer DICOM standard */
      /* return 32bits by default (required) */
      return true;
  }
}

struct _dataelement {
  tag_t tag;
  vr_t vr;
  /*
   * Implementation design. VL is part of the dataelement, since there is a
   * tight relation in between VR and VL.
   */
  vl_t vl;
};

struct _src;
bool read_explicit(struct _src *src, struct _dataelement *de);

static inline size_t get_explicit2_len(struct _dataelement *de) {
  if (isvr32(de->vr)) {
    return 4 + 2;  // yes 4 + 2
  }
  return 2;
}

int read_explicit1(struct _dataelement *de, const char *buf, size_t len);
int read_explicit2(struct _dataelement *de, const char *buf, size_t len);

void print_dataelement(struct _dataelement *de);
