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

#include "dicm-private.h"
#include "dicm-log.h"
#include "dicm-parser.h"

extern struct _log dlog;
extern struct _src fsrc;
extern struct _dst fdst;

#include <assert.h> /* assert */
#include <errno.h>  /* errno */
#include <stdio.h>  /* fopen */
#include <stdlib.h> /* EXIT_SUCCESS */

int main(__maybe_unused int argc, __maybe_unused char *argv[]) {
  set_global_logger(&dlog);

  dst_t fdst;
  dicm_sreader_t *sreader;

  fsrc.ops->open(&fsrc, "input.dcm");
  fdst.ops->open(&fdst, "output.dcm");

  sreader = dicm_sreader_init(&fsrc);
  struct _dataelement de = {0};
  while (dicm_sreader_hasnext(sreader)) {
    int next = dicm_sreader_next(sreader);
    switch (next) {
      case kStartInstance:
        break;

      case kFilePreamble:
        break;

      case kPrefix:
        break;

      case kFileMetaElement:
        dicm_sreader_get_dataelement(sreader, &de);
        print_dataelement(&de);
        break;

      case kDataElement:
        dicm_sreader_get_dataelement(sreader, &de);
        print_dataelement(&de);
        break;

      case kEndInstance:
        break;
    }
  }
  dicm_sreader_fini(sreader);

  fsrc.ops->close(&fsrc);
  fdst.ops->close(&fdst);

  return EXIT_SUCCESS;
}
