// BEGIN DICOM Implementation
static bool read_preamble(FILE * stream)
{
  char buf[4];
  const int ret = fseeko(stream, 128, SEEK_SET );
  if( ret != 0 ) return 0;
  const size_t n = fread( buf, sizeof *buf, sizeof buf, stream );
  return n == 4 && strncmp( buf, "DICM", 4 ) == 0;
}

typedef uint32_t tag_t;
typedef uint16_t vr_t;
typedef uint32_t vl_t;

typedef union { uint16_t tags[2]; tag_t tag; } utag_t;
typedef union { char str[2]; vr_t vr; } uvr_t;
typedef union { char bytes[4]; vl_t vl; } uvl_t;
typedef union { char bytes[2]; uint16_t vl16; } uvl16_t;

static inline uint_fast16_t get_group( tag_t tag )
{
  return (uint16_t)(tag >> 16);
}
static inline uint_fast16_t get_element( tag_t tag )
{
  return (uint16_t)(tag & (uint16_t)0xffff);
}

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
  E_INVALID = 0, /* Item, Item Delimitation Item & Sequence Delimitation Item */
  E_AE = MAKE_VR('A','E'),
  E_AS = MAKE_VR('A','S'),
  E_AT = MAKE_VR('A','T'),
  E_CS = MAKE_VR('C','S'),
  E_DA = MAKE_VR('D','A'),
  E_DS = MAKE_VR('D','S'),
  E_DT = MAKE_VR('D','T'),
  E_FL = MAKE_VR('F','L'),
  E_FD = MAKE_VR('F','D'),
  E_IS = MAKE_VR('I','S'),
  E_LO = MAKE_VR('L','O'),
  E_LT = MAKE_VR('L','T'),
  E_OB = MAKE_VR('O','B'),
  E_OD = MAKE_VR('O','D'),
  E_OF = MAKE_VR('O','F'),
  E_OL = MAKE_VR('O','L'),
  E_OW = MAKE_VR('O','W'),
  E_PN = MAKE_VR('P','N'),
  E_SH = MAKE_VR('S','H'),
  E_SL = MAKE_VR('S','L'),
  E_SQ = MAKE_VR('S','Q'),
  E_SS = MAKE_VR('S','S'),
  E_ST = MAKE_VR('S','T'),
  E_TM = MAKE_VR('T','M'),
  E_UC = MAKE_VR('U','C'),
  E_UI = MAKE_VR('U','I'),
  E_UL = MAKE_VR('U','L'),
  E_UN = MAKE_VR('U','N'),
  E_UR = MAKE_VR('U','R'),
  E_US = MAKE_VR('U','S'),
  E_UT = MAKE_VR('U','T'),
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
  case E_AE:
  case E_AS:
  case E_AT:
  case E_CS:
  case E_DA:
  case E_DS:
  case E_DT:
  case E_FD:
  case E_FL:
  case E_IS:
  case E_LO:
  case E_LT:
  case E_PN:
  case E_SH:
  case E_SL:
  case E_SS:
  case E_ST:
  case E_TM:
  case E_UI:
  case E_UL:
  case E_US:
    /* 16bits: */
    return false;
  case E_OB:
  case E_OD:
  case E_OF:
  case E_OL:
  case E_OW:
  case E_SQ:
  case E_UC:
  case E_UN:
  case E_UR:
  case E_UT:
    /* 32bits: */
    return true;
  default:
    /* parser error, or newer DICOM standard */
    /* return 32bits by default (required) */
    return true;
    }
}

typedef struct
{
  tag_t tag;
  vr_t vr;
  vl_t vl;
} dataelement;

static inline bool tag_equal_to( const dataelement * de, tag_t tag )
{
  return de->tag == tag;
}

static inline bool tag_is_lower( const dataelement * de, tag_t tag )
{
  return de->tag < tag;
}

static inline bool is_start( const dataelement * de )
{
  static const tag_t start = MAKE_TAG( 0xfffe,0xe000 );
  return de->tag == start;
}
static inline bool is_end_item( const dataelement * de )
{
  static const tag_t end_item = MAKE_TAG( 0xfffe,0xe00d );
  return de->tag == end_item;
}
static inline bool is_end_sq( const dataelement * de )
{
  static const tag_t end_sq = MAKE_TAG( 0xfffe,0xe0dd );
  return de->tag == end_sq;
}
static inline bool is_encapsulated_pixel_data( const dataelement * de )
{
  static const tag_t pixel_data = MAKE_TAG( 0x7fe0,0x0010 );
  const bool is_pixel_data = tag_equal_to(de, pixel_data);
  if( is_pixel_data )
    {
    // Make sure Pixel Data is Encapsulated (Sequence of Fragments):
    if( de->vl == (uint32_t)-1 && (de->vr == E_OB || de->vr == E_OW) )
      {
      return true;
      }
    }
  return false;
}
static inline bool is_undef_len( const dataelement * de )
{
  const bool b = de->vl == (uint32_t)-1;
  if( b ) {
    return de->vr == E_SQ || is_encapsulated_pixel_data( de ) || is_start(de);
  }
  return b;
}
static inline uint32_t compute_len( const dataelement * de )
{
  g_assert( !is_undef_len( de ) );
  if( isvr32( de->vr ) )
    {
    return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + de->vl /* VL */;
    }
  return 4 /* tag */ + 4 /* VR/VL */ + de->vl /* VL */;
}
static inline uint32_t compute_undef_len( const dataelement * de, uint32_t len )
{
  g_assert( is_undef_len( de ) );
  g_assert( len != (uint32_t)-1 );
  return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + len;
}
typedef struct {
  tag_t *tags;
  int ntags;
  int size;
} tag_path;

static tag_path * create_tag_path(void)
{
  tag_path *ptr = (tag_path*)malloc( sizeof(tag_path) );
  // for now restrict path of max 16 attributes:
  ptr->tags = malloc( 16 * sizeof(tag_t) );
  ptr->ntags = 0;
  ptr->size = 16;
  return ptr;
}

static void destroy_path(tag_path *tp)
{
  free( tp->tags );
  free( tp );
}

static tag_path * clear_path(tag_path *tp)
{
  tp->ntags = 0;
  return tp;
}

static tag_path * push_tag( tag_path * tp, tag_t t )
{
  if( tp->ntags < tp->size )
    {
    tag_t * ptr = tp->tags + tp->ntags;
    *ptr = t;
    tp->ntags++;
    }
  else
    {
    // programmer error, stack exhausted
    g_assert(0);
    }
  return tp;
}

static tag_t pop_tag( tag_path * tp )
{
  tp->ntags--;
  const tag_t * ptr = tp->tags + tp->ntags;
  return *ptr;
}

static tag_t last_tag( tag_path * tp )
{
  const tag_t * ptr = tp->tags + (tp->ntags - 1);
  return *ptr;
}

static bool tag_path_equal_to( tag_path * tp1, tag_path * tp2 )
{
  if( tp1->ntags == tp2->ntags )
    {
    int t;
    for( t = 0; t < tp1->ntags; ++t )
      {
      if( tp1->tags[t] != tp2->tags[t] ) return false;
      }
    // ok
    return true;
    }
  return false;
}

typedef struct {
  tag_t *tags;
  int size; /* max number of tags */
  int *ntags;
  int nsets;
} tag_path_set;

static tag_path_set * create_tag_path_set(void)
{
  tag_path_set *ptr = (tag_path_set*)malloc( sizeof(tag_path_set) );
  ptr->tags = malloc( 512 * sizeof(tag_t) );
  ptr->size = 512;
  ptr->ntags = malloc( 16 * sizeof(int) );
  ptr->nsets = 0;
  return ptr;
}

static void destroy_path_set( tag_path_set * tps )
{
  free( tps->ntags );
  free( tps->tags );
  free( tps );
}

static bool find_tag_path( tag_path_set *tps, tag_path *tp ) WUR;
static bool find_tag_path( tag_path_set *tps, tag_path *tp )
{
  int s;
  int total = 0;
  tag_path ref;
  for( s = 0; s < tps->nsets; ++s )
    {
    tag_t * ptr = tps->tags + total;
    ref.tags = ptr;
    ref.ntags = ref.size = tps->ntags[s];

    const bool b = tag_path_equal_to( &ref, tp );
    if( b ) return true;
    total += tps->ntags[s];
    }
  return false;
}

static void add_tag_path( tag_path_set *tps, tag_path *tp )
{
  int s;
  int total = 0;
  for( s = 0; s < tps->nsets; ++s )
    {
    total += tps->ntags[s];
    }
  g_assert( total <= tps->size );
  tag_t * ptr = tps->tags + total;
  for( s = 0; s < tp->ntags; ++s )
    {
    ptr[s] = tp->tags[s];
    }
  tps->ntags[ tps->nsets ] = tp->ntags;
  tps->nsets++;
  g_assert( find_tag_path( tps, tp ) );
}

typedef struct dataset dataset;
struct dataset {
  tag_path *cur_tp;
  tag_path_set *tps;
  void (*handle_attribute)( dataset * ds, FILE * stream, uint32_t len );
  void (*handle_pixel_data_item)( dataset * ds, FILE * stream, uint32_t len);
  void *data;
};

// explicit
static bool read_explicit( dataelement * de, FILE * stream ) WUR;
static bool read_explicit( dataelement * de, FILE * stream )
{
  utag_t t;
  uvr_t vr;
  uvl_t vl;

  // Tag
  size_t n = fread( t.tags, sizeof *t.tags, 2, stream );
  if( n != 2 ) return false;
  SWAP_TAG(t);
  if( !tag_is_lower( de, t.tag ) ) return false;

  // Value Representation
  n = fread( vr.str, sizeof *vr.str, 2, stream );
  /* a lot of VR are not valid (eg: non-ASCII), however the standard may add
   * them in a future edition, so only exclude the impossible ones */
  if( n != 2 || !isvr_valid(vr) ) return false;

  // padding and/or 16bits VL
  uvl16_t vl16;
  n = fread( vl16.bytes, sizeof *vl16.bytes, 2, stream );
  if( n != 2 ) return false;

  // Value Length
  if( isvr32( vr.vr ) )
    {
    /* padding must be set to zero */
    if( vl16.vl16 != 0 ) return false;

    n = fread( vl.bytes, 1, 4, stream );
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

static bool read_explicit_undef( dataelement * de, FILE * stream ) WUR;
static bool read_explicit_undef( dataelement * de, FILE * stream )
{
  utag_t t;
  uvr_t vr;
  uvl_t vl;

  // Tag
  size_t n = fread( t.tags, sizeof *t.tags, 2, stream );
  if( n != 2 ) return false;
  SWAP_TAG(t);
  if( !tag_is_lower( de, t.tag ) ) return false;

  static const tag_t item_end = MAKE_TAG( 0xfffe,0xe00d );
  if( t.tag == item_end )
    {
    // Special case:
    n = fread( vl.bytes, sizeof *vl.bytes, 4, stream );
    if( n != 4 || vl.vl != 0 ) return false;
    vr.vr = E_INVALID;
    }
  else
    {
    // Value Representation
    if( get_group(t.tag) == 0xfffe ) return false;
    n = fread( vr.str, sizeof *vr.str, 2, stream );
    if( n != 2 || !isvr_valid(vr) ) return false;

    // padding and/or 16bits VL
    uvl16_t vl16;
    n = fread( vl16.bytes, sizeof *vl16.bytes, 2, stream );
    if( n != 2 ) return false;

    // Value Length
    if( isvr32( vr.vr ) )
      {
      /* padding must be set to zero */
      if( vl16.vl16 != 0 ) return false;

      n = fread( vl.bytes, sizeof *vl.bytes, 4, stream );
      if( n != 4 ) return false;
      SWAP_VL(vl.vl);
      }
    else
      {
      SWAP_VL16(vl16.vl16);
      vl.vl = vl16.vl16;
      }
    }
  de->tag = t.tag;
  de->vr  = vr.vr;
  de->vl  = vl.vl;
  return true;
}

// implicit
static bool read_implicit( dataelement * de, FILE * stream ) WUR;
static bool read_implicit( dataelement * de, FILE * stream )
{
  utag_t t;
  uvl_t vl;
  // Tag
  size_t n = fread( t.tags, sizeof *t.tags, 2, stream );
  if( n != 2 ) return false;
  SWAP_TAG(t);
  if( !tag_is_lower( de, t.tag ) ) return false;

  // Value Length (always 32bits)
  n = fread( vl.bytes, 1, 4, stream );
  if( n != 4 ) return false;
  SWAP_VL(vl.vl);

  de->tag = t.tag;
  de->vr  = E_INVALID;
  de->vl  = vl.vl;
  return true;
}

static inline bool read_ul( FILE * stream, uint32_t * value ) WUR;
static inline bool read_ul( FILE * stream, uint32_t * value )
{
  if( fread( (char*)value, sizeof *value, 1, stream ) != 1 )
    return false;
  SWAP_VL(*value);
  return true;
}

// read header
static bool read_meta( FILE * stream ) WUR;
static bool read_meta( FILE * stream )
{
  dataelement de = { 0 /* tag */ };
  if( !read_explicit( &de, stream ) ) return false;
  if( de.tag != MAKE_TAG(0x2,0x0)
    || de.vr != E_UL
    || de.vl != 4 ) return false;
  // file meta group length
  uint32_t gl;
  if( !read_ul( stream, &gl ) ) return false;
  // for now skip the meta header (we may check extra info here one day)
  const int ret = fseeko( stream, gl, SEEK_CUR );
  return ret == 0;
}

static void process_attribute( dataset *ds, dataelement *de, FILE * stream )
{
  g_assert( !is_start(de) && !is_end_item(de) && !is_end_sq(de) );
  if( is_undef_len(de) )
    {
    if( ds->handle_attribute ) (ds->handle_attribute)(ds, NULL, de->vl);
    }
  else
    {
    if( ds->handle_attribute ) (ds->handle_attribute)(ds, stream, de->vl);
    }
}

static uint32_t read_sq_undef( dataset * ds, FILE * stream ) WUR;
static uint32_t read_encapsulated_pixel_data( dataset * ds, FILE * stream ) WUR;

/* read a single undefined length Item */
/* return 0 upon failure, which is an invalid length for Undefined Length Item */
static uint32_t read_item_undef( dataset * ds, FILE * stream ) WUR;
static uint32_t read_item_undef( dataset * ds, FILE * stream )
{
  dataelement de = { 0 /* tag */ };
  uint32_t itemlen = 0;
  do
    {
    // carefully read an Item End or an Explicit Data Element:
    if( !read_explicit_undef(&de, stream) ) return 0;
    if( is_end_item(&de) )
      {
      // end of Item
      itemlen += 4 /* tags */ + 4 /* VL */;
      break;
      }

    push_tag( ds->cur_tp, de.tag );
    process_attribute( ds, &de, stream );
    if( is_undef_len(&de) )
      {
      // Either undefined SQ or encapsulated Pixel Data:
      const bool is_encapsulated = is_encapsulated_pixel_data( &de );
      if( is_encapsulated )
        {
        const uint32_t epdlen = read_encapsulated_pixel_data( ds, stream );
        if( epdlen == 0 ) return 0;
        itemlen += compute_undef_len( &de, epdlen );
        }
      else
        {
        if( de.vr != E_SQ ) return 0; // IVRLE
        const uint32_t seqlen = read_sq_undef( ds, stream );
        if( seqlen == 0 ) return 0;
        itemlen += compute_undef_len( &de, seqlen );
        }
      }
    else
      {
      itemlen += compute_len( &de );
      }
    pop_tag( ds->cur_tp );
    } while( 1 );

  g_assert( itemlen > 0 );
  return itemlen;
}

/* read a single defined length Item of length: itemlen */
static bool read_item_def( dataset * ds, FILE * stream, const uint32_t itemlen ) WUR;
static bool read_item_def( dataset * ds, FILE * stream, const uint32_t itemlen )
{
  uint32_t curlen = 0;
  dataelement de = { 0 /* tag */ };
  while( curlen != itemlen )
    {
    if( curlen > itemlen ) return false;
    // read attribute
    if( !read_explicit(&de, stream) ) return false;
    push_tag( ds->cur_tp, de.tag );
    process_attribute( ds, &de, stream );
    if( is_undef_len(&de) )
      {
      // If undefined SQ or encapsulated Pixel Data:
      const bool is_encapsulated = is_encapsulated_pixel_data( &de );
      if( is_encapsulated )
        {
        const uint32_t epdlen = read_encapsulated_pixel_data( ds, stream );
        if( epdlen == 0 ) return false;
        curlen += compute_undef_len( &de, epdlen );
        }
      else
        {
        const uint32_t seqlen = read_sq_undef( ds, stream );
        if( seqlen == 0 ) return false;
        curlen += compute_undef_len( &de, seqlen );
        }
      }
    else
      {
      curlen += compute_len( &de );
      }
    pop_tag( ds->cur_tp );
    }
  return true;
}

/* read a single undefined length SQ */
/* return the actual length of the sequence */
/* return 0 upon error, which is an impossible length for an undefined length Sequence */
static uint32_t read_sq_undef( dataset * ds, FILE * stream )
{
  dataelement de;
  uint32_t seqlen = 0;
  do
    {
    // restart tag for each new Item:
    de.tag = 0;
    // Item start
    if( !read_implicit( &de, stream ) ) return 0;
    if( is_end_sq(&de) )
      {
      /* end of SQ */
      if( de.vl != 0 ) return 0;
      seqlen += 4 /* tag */ + 4 /* vl */;
      break;
      }
    if( !is_start(&de) ) return 0;

    if( is_undef_len(&de) )
      {
      const uint32_t itemlen = read_item_undef( ds, stream );
      if( itemlen == 0 ) return 0;
      seqlen += 4 /* tag */ + 4 /* vl */ + itemlen;
      }
    else
      {
      if( !read_item_def( ds, stream, de.vl ) ) return 0;
      seqlen += 4 /* tag */ + 4 /* vl */ + de.vl;
      }
    } while( 1 );

  // post-condition
  if( !is_end_sq(&de) ) return 0;

  g_assert( seqlen > 0 );
  return seqlen;
}

/* Encapsulated Pixel Data (one JPEG stream per Item) */
/* return 0 upon error */
static uint32_t read_encapsulated_pixel_data( dataset * ds, FILE * stream )
{
  dataelement de;
  uint32_t epdlen = 0;
  do
    {
    /* restart for each Item */
    de.tag = 0;
    /* Item start */
    if( !read_implicit(&de, stream) ) return 0;
    /* end of SQ */
    epdlen += 4 /* tag */ + 4 /* vl */;
    if( is_end_sq(&de) ) break;
    if( !is_start(&de) ) return 0;

    if( ds->handle_pixel_data_item )  (ds->handle_pixel_data_item)( ds, stream, de.vl );
    epdlen += de.vl;
    } while( 1 );

  // post-condition
  if( !is_end_sq(&de) ) return 0;

  g_assert( epdlen > 0 );
  return epdlen;
}

/* read a single defined length SQ */
/* return true on success */
static bool read_sq_def( dataset * ds, FILE * stream, const uint32_t seqlen ) WUR;
static bool read_sq_def( dataset * ds, FILE * stream, const uint32_t seqlen )
{
  uint32_t curlen = 0;
  dataelement de;
  while( curlen != seqlen )
    {
    // illegal:
    if( curlen > seqlen ) return false;
    // restart for each Item:
    de.tag = 0;
    // start
    if( !read_implicit( &de, stream ) || !is_start(&de) ) return false;

    if( is_undef_len(&de) )
      {
      const size_t itemlen = read_item_undef( ds, stream );
      if( itemlen == 0 ) return false;
      curlen += 4 /* tag */ + 4 /* VL */ + itemlen;
      }
    else
      {
      curlen += 4 /* tag */ + 4 /* VL */ + de.vl;
      if( !read_item_def( ds, stream, de.vl ) ) return false;
      }
    }
  return true;
}

// Main loop
static bool read_dataset( dataset * ds, FILE * stream ) WUR;
static bool read_dataset( dataset * ds, FILE * stream )
{
  dataelement de = { 0 /* tag */ };
  while( read_explicit( &de, stream ) )
    {
    if( get_group( de.tag ) == 0xfffe ) return false;
    if( get_group( de.tag )  > 0x7fe0 ) return false;
    push_tag( ds->cur_tp, de.tag );
    if( is_undef_len(&de) )
      {
      process_attribute( ds, &de, stream );
      if( de.vr != E_SQ )
        {
        if( !is_encapsulated_pixel_data( &de ) ) return false;
        if( de.vr == E_UN ) // IVRLE !
          {
          // Technically a Sequence could be stored with VR:UN, this does
          // not make sense in the WSMIS world, thus it is not handled.
          return false;
          }
        const uint32_t pdlen = read_encapsulated_pixel_data( ds, stream );
        if( pdlen == 0 ) return false;
        }
      else
        {
        if( de.vr != E_SQ ) return false;
        const uint32_t seqlen = read_sq_undef(ds, stream);
        if( seqlen == 0 ) return false;
        }
      }
    else
      {
      if( de.vr == E_SQ )
        {
        if( !read_sq_def( ds, stream, de.vl ) ) return false;
        }
      else
        {
        process_attribute( ds, &de, stream );
        }
      }
    pop_tag( ds->cur_tp );
    }

  // let's be forgiving with extra tailing garbage:
  // g_assert( feof( stream ) );
  return true;
}
