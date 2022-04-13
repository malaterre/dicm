/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-features.h"

//#include <sys/types.h> /* off_t */
#include <stdint.h> /* uint16_t */

enum state {
#if 0
  // http://dicom.nema.org/medical/dicom/current/output/chtml/part10/chapter_7.html#table_7.1-1
  kStartFileMetaInformation = 0,
  /**
   * Only when stream_filemetaelements is true
   */
  kFilePreamble,
  /**
   * Only when stream_filemetaelements is true
   */
  kDICOMPrefix,
  /**
   * Only when stream_filemetaelements is true
   */
  kFileMetaInformationGroupLength,
  /**
   * Only when stream_filemetaelements is true
   */
  kFileMetaElement,
  kEndFileMetaInformation,
#endif
#if 0
  kDataElement,             // Implicit or Explicit
  //kGroupLengthDataElement,  // PS 3.5 §7.2
  //kEndGroupDataElement,
  kSequenceOfItems,
  kSequenceOfFragments,
  kItem,                                 // (FFFE,E000)
  kFragment,                             // (FFFE,E000)
  kItemDelimitationItem,                 // (FFFE,E00D)
  kSequenceOfItemsDelimitationItem,      // (FFFE,E0DD)
  kSequenceOfFragmentsDelimitationItem,  // (FFFE,E0DD)
  kBasicOffsetTable,  // First Item in a Sequence of Fragments
#endif
  kStartModel = 0,
  kEndModel,
  kStartAttribute,
  kEndAttribute,
  kValue,
  kStartFragment,
  kEndFragment,
  kStartItem,
  kEndItem,
  kStartSequence,
  kEndSequence,
};

#define _DICM_POISON(replacement) error__use_##replacement##_instead
//#define fseek _DICM_POISON(fseeko)
//#define ftell _DICM_POISON(ftello)
#define strtod _DICM_POISON(_dicm_parse_double)

typedef char byte_t;
// Fast access to Tag (group, element)
typedef uint32_t tag_t;
// A value representation (upper case ASCII)
typedef byte_t(vr_t)[2];
// A value length (-1 means undefined)
typedef uint32_t vl_t;
