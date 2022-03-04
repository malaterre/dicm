/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-de.h"
#include "dicm-errno.h"
#include "dicm-features.h"
#include "dicm-io.h"

// struct _dicm_sreader;

#if 0
struct _dicm_filepreamble {
  byte_t data[128];
};

struct _dicm_prefix {
  byte_t data[4];
};
#endif

#if 0
struct reader_prv_vtable {
  int (*fp_write)(void *const, const struct dicm_data_element *de);
};

/* common writer vtable */
struct writer_vtable {
  struct object_prv_vtable const object;
  struct writer_prv_vtable const writer;
};

/* common writer object */
struct dicm_writer {
  struct writer_vtable const *vtable;
};

/* common writer interface */
#define dicm_writer_write(t, de) ((t)->vtable->writer.fp_write((t), (de)))
#endif

struct dicm_sreader;
// DICM_EXPORT struct _dicm_sreader *dicm_sreader_init();
DICM_EXPORT int dicm_sreader_create(struct dicm_sreader **pself,
                                    struct dicm_io *src);

/* DICM_EXPORT void dicm_sreader_set_src(struct _dicm_sreader *sreader,
                                      struct dicm_io *src);*/
// DICM_EXPORT int dicm_sreader_destroy(struct _dicm_sreader *sreader);

// DICM_EXPORT void dicm_sreader_stream_filemetaelements( struct _dicm_sreader
// *sreader, bool stream_filemetaelements); DICM_EXPORT void
// dicm_sreader_group_length(struct _dicm_sreader *sreader, bool group_length);

/**
 * Read file meta info (preamble, prefix, file meta elements)
 */
// DICM_CHECK_RETURN bool dicm_sreader_read_meta_info(struct dicm_sreader
// *sreader);

// DICM_EXPORT int dicm_sreader_fini(struct dicm_sreader *sreader);

/**
 * Indicate whether or not there is a next dataelement
 */
DICM_EXPORT DICM_CHECK_RETURN bool dicm_sreader_hasnext(
    struct dicm_sreader *sreader);

/**
 * Move to next dataelement
 */
DICM_EXPORT int dicm_sreader_next(struct dicm_sreader *sreader);

/**
 * Return DICOM File Preamble
 */
// DICM_EXPORT DICM_CHECK_RETURN bool dicm_sreader_get_file_preamble(
// struct _dicm_sreader *sreader, struct _dicm_filepreamble *filepreamble);

/**
 * Return DICOM Prefix
 */
// DICM_EXPORT DICM_CHECK_RETURN bool dicm_sreader_get_prefix(
// struct _dicm_sreader *sreader, struct _dicm_prefix *prefix);

/**
 * Return current filedataelement
 */
// DICM_EXPORT DICM_CHECK_RETURN bool dicm_sreader_get_filemetaelement(
// struct _dicm_sreader *sreader, struct _filemetaelement *fme);

/**
 * Return current dataelement
 */
// DICM_EXPORT DICM_CHECK_RETURN bool dicm_sreader_get_dataelement(
// struct _dicm_sreader *sreader, struct _dataelement *de);
DICM_EXPORT bool dicm_sreader_get_tag(struct dicm_sreader *sreader, tag_t *tag);
DICM_EXPORT bool dicm_sreader_get_vr(struct dicm_sreader *sreader, vr_t *vr);
DICM_EXPORT bool dicm_sreader_get_vl(struct dicm_sreader *sreader, vl_t *vl);
DICM_EXPORT bool dicm_sreader_get_value(struct dicm_sreader *sreader, void *buf,
                                        vl_t len);

/**
 * Return current dataelement value.
 */
/* DICM_EXPORT size_t dicm_sreader_pull_dataelement_value(
    struct _dicm_sreader *sreader, const struct _dataelement *de, char *buf,
    size_t buflen);*/

// struct _dataset *dicm_sreader_get_dataset(struct _dicm_sreader *sreader);

/** DICM stream reader */
typedef struct _dicm_sreader dicm_sreader_t;
