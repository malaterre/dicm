#include "dicm-de.h"

#include <stdlib.h>
#include <stdio.h>

static inline const char * get_vr2(uint16_t val)
{
  return (const char*)&val;
}

typedef char (bytes2_t)[2];

//static inline bytes2_t get_vr3(uint16_t val)
//{
//  bytes2_t b = { 'A', 'B' };
//  return b;
//}

typedef union {
  char bytes[2];
  uint16_t val;
} uval_t;

struct opaque;
static inline uval_t get_val_impl(struct opaque *opaque) {
  uval_t uval;
  uval.val = 16961;
  return uval;
}

struct opaque {};

#define get_val(val) get_val_impl(val).bytes


int testdicm_de(__maybe_unused int argc, __maybe_unused char *argv[])
{
  printf("tag_t: %lu\n", sizeof(tag_t) );
  printf("vr_t: %lu\n", sizeof(vr_t) );
  printf("vl_t: %lu\n", sizeof(vl_t) );
  printf("de: %lu\n", sizeof(struct _dataelement) );

    struct {
      utag_t utag;
      uvr32_t uvr;
      uvl_t uvl;
    } ede;  // explicit data element
    struct {
      utag_t utag;
      uvr_t uvr;
      uvl16_t uvl;
    } ede16;  // explicit data element, VR 32
    struct {
      utag_t utag;
      uvl_t uvl;
    } ide;  // implicit data element

  printf("ede: %lu\n", sizeof ede );
  printf("ede16: %lu\n", sizeof ede16 );
  printf("ide: %lu\n", sizeof ide );

  uvr_t uvr;
  uvr.bytes[0] = 'A';
  uvr.bytes[1] = 'B';
  printf("%d\n", uvr.vr);
  printf("%.2s\n", get_vr(uvr.vr));
  printf("%.2s\n", get_vr2(uvr.vr));

  bytes2_t b = { 'A', 'B' };
  printf("%.2s\n", b);

//  printf("%.2s\n", get_vr3(uvr.vr));

  struct opaque opaque;
  printf("%.2s\n", get_val(&opaque));

  uint16_t val = 16961;
  printf("%.2s\n", (const char*)&val);


  return EXIT_SUCCESS;
}
