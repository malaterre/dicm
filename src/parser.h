#pragma once

#include "dicm-private.h"

#include <stdbool.h> /* bool */
#include <stddef.h> /* size_t */
#include <stdint.h> /* uint16_t */


//struct _parser {
//  char filepreamble[128];
//  char prefix[4];
//  struct _dataelement filemetaelement;
//  struct _dataelement dataelement;
//};
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
static inline const char *get_vr( vr_t vr )
{
  uvr_t *uvr = (uvr_t*)&vr;
  return uvr->str;
}


struct _dataelement
{
  tag_t tag;
  vr_t vr;
  /*
   * Implementation design. VL is part of the dataelement, since there is a tight relation in between VR and VL.
   */
  vl_t vl;
};


typedef struct _dataelement dataelement_t;


struct _src;
bool read_explicit( struct _src *src, struct _dataelement * de );

void print_dataelement(struct _dataelement *de);
