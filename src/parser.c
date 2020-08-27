#include "parser.h"

#include <assert.h>
#include <stdio.h>

#define MAKE_TAG(group,element) (group << 16 | element)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAP_TAG(t) t.tag = MAKE_TAG(t.tags[0], t.tags[1])
#else
#define SWAP_TAG(t) t.tags[0] = bswap_16( t.tags[0] ); t.tags[1] = bswap_16( t.tags[1] )
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MAKE_VR(left,right) (right << 8 | left)
#else
#define MAKE_VR(left,right) (left << 8 | right)
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
  kAE = MAKE_VR('A','E'),
  kAS = MAKE_VR('A','S'),
  kAT = MAKE_VR('A','T'),
  kCS = MAKE_VR('C','S'),
  kDA = MAKE_VR('D','A'),
  kDS = MAKE_VR('D','S'),
  kDT = MAKE_VR('D','T'),
  kFL = MAKE_VR('F','L'),
  kFD = MAKE_VR('F','D'),
  kIS = MAKE_VR('I','S'),
  kLO = MAKE_VR('L','O'),
  kLT = MAKE_VR('L','T'),
  kOB = MAKE_VR('O','B'),
  kOD = MAKE_VR('O','D'),
  kOF = MAKE_VR('O','F'),
  kOL = MAKE_VR('O','L'),
  kOW = MAKE_VR('O','W'),
  kPN = MAKE_VR('P','N'),
  kSH = MAKE_VR('S','H'),
  kSL = MAKE_VR('S','L'),
  kSQ = MAKE_VR('S','Q'),
  kSS = MAKE_VR('S','S'),
  kST = MAKE_VR('S','T'),
  kTM = MAKE_VR('T','M'),
  kUC = MAKE_VR('U','C'),
  kUI = MAKE_VR('U','I'),
  kUL = MAKE_VR('U','L'),
  kUN = MAKE_VR('U','N'),
  kUR = MAKE_VR('U','R'),
  kUS = MAKE_VR('U','S'),
  kUT = MAKE_VR('U','T'),
};

static inline bool isvr_valid( const uvr_t uvr )
{
  if( uvr.str[0] < 'A' || uvr.str[0] > 'Z' ) return false;
  if( uvr.str[1] < 'A' || uvr.str[1] > 'Z' ) return false;
  return true;
}

static inline bool isvr32( const vr_t vr )
{
  switch( vr )
    {
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

static inline bool tag_equal_to( const struct _dataelement * de, tag_t tag )
{
  return de->tag == tag;
}

static inline bool tag_is_lower( const struct _dataelement * de, tag_t tag )
{
  return de->tag < tag;
}

static inline bool is_start( const struct _dataelement * de )
{
  static const tag_t start = MAKE_TAG( 0xfffe,0xe000 );
  return de->tag == start;
}
static inline bool is_end_item( const struct _dataelement * de )
{
  static const tag_t end_item = MAKE_TAG( 0xfffe,0xe00d );
  return de->tag == end_item;
}
static inline bool is_end_sq( const struct _dataelement * de )
{
  static const tag_t end_sq = MAKE_TAG( 0xfffe,0xe0dd );
  return de->tag == end_sq;
}
static inline bool is_encapsulated_pixel_data( const struct _dataelement * de )
{
  static const tag_t pixel_data = MAKE_TAG( 0x7fe0,0x0010 );
  const bool is_pixel_data = tag_equal_to(de, pixel_data);
  if( is_pixel_data )
    {
    // Make sure Pixel Data is Encapsulated (Sequence of Fragments):
    if( de->vl == (uint32_t)-1 && (de->vr == kOB || de->vr == kOW) )
      {
      return true;
      }
    }
  return false;
}
static inline bool is_undef_len( const struct _dataelement * de )
{
  const bool b = de->vl == (uint32_t)-1;
  if( b ) {
    return de->vr == kSQ || is_encapsulated_pixel_data( de ) || is_start(de);
  }
  return b;
}
static inline uint32_t compute_len( const struct _dataelement * de )
{
  assert( !is_undef_len( de ) );
  if( isvr32( de->vr ) )
    {
    return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + de->vl /* VL */;
    }
  return 4 /* tag */ + 4 /* VR/VL */ + de->vl /* VL */;
}
static inline uint32_t compute_undef_len( const struct _dataelement * de, uint32_t len )
{
  assert( is_undef_len( de ) );
  assert( len != (uint32_t)-1 );
  return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + len;
}

bool read_explicit( struct _src *src, struct _dataelement * de )
{
  utag_t t;
  uvr_t vr;
  uvl_t vl;

  // Tag
  //size_t n = fread( t.tags, sizeof *t.tags, 2, stream );
  size_t n = src->ops->read(src, t.tags, sizeof *t.tags * 2 );
  if( n != 4 ) return false;
  SWAP_TAG(t);
  if( !tag_is_lower( de, t.tag ) ) return false;

  // Value Representation
  //n = fread( vr.str, sizeof *vr.str, 2, stream );
  n = src->ops->read(src, vr.str, sizeof *vr.str * 2 );
  /* a lot of VR are not valid (eg: non-ASCII), however the standard may add
   * them in a future edition, so only exclude the impossible ones */
  if( n != 2 || !isvr_valid(vr) ) return false;

  // padding and/or 16bits VL
  uvl16_t vl16;
  //n = fread( vl16.bytes, sizeof *vl16.bytes, 2, stream );
  n = src->ops->read(src, vl16.bytes, sizeof *vl16.bytes * 2 );
  if( n != 2 ) return false;

  // Value Length
  if( isvr32( vr.vr ) )
    {
    /* padding must be set to zero */
    if( vl16.vl16 != 0 ) return false;

    // n = fread( vl.bytes, 1, 4, stream );
    n = src->ops->read(src, vl.bytes, 1 * 4 );
    if( n != 4 ) return false;
    SWAP_VL(vl.vl);
    }
  else
    {
    SWAP_VL16(vl16.vl16);
    vl.vl = vl16.vl16;
    }
  de->tag = t.tag;
  de->vr  = vr.vr;
  de->vl  = vl.vl;
  return true;
}

bool read_explicit1( struct _dataelement * de, const char * buf, size_t len )
{
  utag_t t;
  uvr_t vr;
  uvl_t vl;

  // Tag
  //size_t n = fread( t.tags, sizeof *t.tags, 2, stream );
  size_t n = src->ops->read(src, t.tags, sizeof *t.tags * 2 );
  if( n != 4 ) return false;
  SWAP_TAG(t);
  if( !tag_is_lower( de, t.tag ) ) return false;

  // Value Representation
  //n = fread( vr.str, sizeof *vr.str, 2, stream );
  n = src->ops->read(src, vr.str, sizeof *vr.str * 2 );
  /* a lot of VR are not valid (eg: non-ASCII), however the standard may add
   * them in a future edition, so only exclude the impossible ones */
  if( n != 2 || !isvr_valid(vr) ) return false;

  // padding and/or 16bits VL
  uvl16_t vl16;
  //n = fread( vl16.bytes, sizeof *vl16.bytes, 2, stream );
  n = src->ops->read(src, vl16.bytes, sizeof *vl16.bytes * 2 );
  if( n != 2 ) return false;

  // Value Length
  if( isvr32( vr.vr ) )
    {
    /* padding must be set to zero */
    if( vl16.vl16 != 0 ) return false;

    // n = fread( vl.bytes, 1, 4, stream );
    n = src->ops->read(src, vl.bytes, 1 * 4 );
    if( n != 4 ) return false;
    SWAP_VL(vl.vl);
    }
  else
    {
    SWAP_VL16(vl16.vl16);
    vl.vl = vl16.vl16;
    }
  de->tag = t.tag;
  de->vr  = vr.vr;
  de->vl  = vl.vl;
  return true;
}

void print_dataelement(struct _dataelement *de)
{
//  utag_t t;
//  uvr_t vr;
//  uvl_t vl;
//  t.tag = de->tag;
//  vr.vr = de->vr;
//  vl.vl = de->vl;

  printf( "%04x,%04x %c%c\n", (unsigned int)get_group(de->tag), (unsigned int)get_element(de->tag),
    get_vr(de->vr)[0], get_vr(de->vr)[1] );
}
