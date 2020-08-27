#pragma once

//#include <sys/types.h> /* off_t */

enum state {
  kStartInstance = 0,
  // http://dicom.nema.org/medical/dicom/current/output/chtml/part10/chapter_7.html#table_7.1-1
  kFilePreamble,
  kPrefix,
  kFileMetaElement,
  kDataElement,
  kEndInstance
};


