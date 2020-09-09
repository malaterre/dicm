#include "dicm-de.h"

#include <stdlib.h>
#include <stdio.h>

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

  return EXIT_SUCCESS;
}
