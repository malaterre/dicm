#include "dicm-public.h"

#include <stdlib.h> /* EXIT_SUCCESS */
#include <string.h>

int testdicm_vr(DICM_UNUSED int argc, DICM_UNUSED char *argv[]) {
  dicm_vr_t vr;
  vr = VR_AE;
  const char *str = dicm_vr_get_string(vr);
  if (strcmp(str, "AE")) return 1;

  return EXIT_SUCCESS;
}
