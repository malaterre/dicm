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
#include "writer.h"

#include "dicm-private.h"
#include <stdio.h>
#include <stdlib.h>

struct _json {
  struct dicm_writer writer;
  /* data */
  const char *separator;
  bool pretty;
  int indent_level;
  dicm_vr_t vr;
};

/* object */
static DICM_CHECK_RETURN int _json_destroy(void *self_) DICM_NONNULL;

/* writer */
static DICM_CHECK_RETURN int _json_write_start_attribute(
    void *self, const struct dicm_attribute *da) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_attribute(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_value(void *self, const void *buf,
                                               size_t s) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_start_fragment(
    void *self, int frag_num) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_fragment(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_start_item(void *self,
                                                    int item_num) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_item(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_start_sequence(
    void *self, const struct dicm_attribute *da) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_sequence(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_start_model(
    void *self, const char *encoding) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_model(void *self) DICM_NONNULL;

static struct writer_vtable const g_vtable =
    {/* object interface */
     .object = {.fp_destroy = _json_destroy},
     /* writer interface */
     .writer = {
         .fp_write_start_attribute = _json_write_start_attribute,
         .fp_write_end_attribute = _json_write_end_attribute,
         .fp_write_value = _json_write_value,
         .fp_write_start_fragment = _json_write_start_fragment,
         .fp_write_end_fragment = _json_write_end_fragment,
         .fp_write_start_item = _json_write_start_item,
         .fp_write_end_item = _json_write_end_item,
         .fp_write_start_sequence = _json_write_start_sequence,
         .fp_write_end_sequence = _json_write_end_sequence,
         .fp_write_start_model = _json_write_start_model,
         .fp_write_end_model = _json_write_end_model,
     }};

int dicm_json_writer_create(struct dicm_writer **pself) {
  struct _json *self = (struct _json *)malloc(sizeof(*self));
  if (self) {
    *pself = &self->writer;
    self->writer.vtable = &g_vtable;
    self->separator = NULL;
    self->pretty = true;
    self->indent_level = 0;
    return 0;
  }
  return 1;
}

/* object */
int _json_destroy(void *self_) {
  struct _json *self = (struct _json *)self_;
  free(self);
  return 0;
}

static void print_with_indent(int indent, char *string) {
  printf("%*s%s", indent, "", string);
}

static void print_indent(void *self_) {
  struct _json *self = (struct _json *)self_;
  if (self->pretty) {
    int level = 2 * self->indent_level;
    print_with_indent(level, "");
  }
}
static void print_eol(void *self_) {
  struct _json *self = (struct _json *)self_;
  if (self->pretty) {
    printf("\n");
  }
}

static void print_separator(void *self_) {
  struct _json *self = (struct _json *)self_;
  if (self->separator) {
    if (self->pretty) printf(self->separator);
  }
}

/* writer */
int _json_write_start_attribute(void *self_, const struct dicm_attribute *da) {
  struct _json *self = (struct _json *)self_;
  print_separator(self_);
  print_indent(self_);

  unsigned int i = get_group(da->tag);
  unsigned int j = get_element(da->tag);
  dicm_vr_t vr = da->vr;
  const char *s = (char *)&vr;
  printf("\"%04X%04X\": {", (unsigned int)get_group(da->tag),
         (unsigned int)get_element(da->tag));
  self->indent_level++;
  print_eol(self_);
  print_indent(self_);
  printf("\"vr\": \"%.2s\",", s);
  print_eol(self_);
  print_indent(self_);
  printf("\"Value\": [");
  print_eol(self_);

  self->separator = ",\n";
  self->indent_level++;
  self->vr = vr;
  return 0;
}
int _json_write_end_attribute(void *self_) {
  struct _json *self = (struct _json *)self_;
  self->indent_level--;
  print_eol(self_);
  print_indent(self_);
  printf("]");
  print_eol(self_);
  self->indent_level--;
  print_indent(self_);
  printf("}");
  return 0;
}
int _json_write_value(void *self_, const void *buf, size_t s) {
  struct _json *self = (struct _json *)self_;
  print_indent(self_);
  const dicm_vr_t vr = self->vr;
  const char *str = (char *)&vr;
  if (vr == 21315) {  // CS
    const char *value = buf;
    printf("%.*s", s, value);
  } else {
    printf("coucou");
  }
  return 0;
}
int _json_write_start_fragment(void *self, int frag_num) { return 0; }
int _json_write_end_fragment(void *self) { return 0; }
int _json_write_start_item(void *self, int item_num) { return 0; }
int _json_write_end_item(void *self) { return 0; }
int _json_write_start_sequence(void *self, const struct dicm_attribute *da) {
  return 0;
}
int _json_write_end_sequence(void *self) { return 0; }
int _json_write_start_model(void *self_, const char *encoding) {
  struct _json *self = (struct _json *)self_;
  printf("{");
  print_eol(self);
  self->indent_level++;

  return 0;
}
int _json_write_end_model(void *self) { return 0; }

#if 0
static const char *separator = NULL;

static void print_separator() {
  if (separator) printf(separator);
}
static void json_start_fmi(DICM_UNUSED struct _writer *writer) {}

static void json_end_fmi(DICM_UNUSED struct _writer *writer) { printf("{\n"); }

static void json_fmi_gl(DICM_UNUSED struct _writer *writer,
                        DICM_UNUSED uint32_t gl) {}

static void json_group_gl(DICM_UNUSED struct _writer *writer,
                          DICM_UNUSED uint32_t gl) {}

static void json_end_group(DICM_UNUSED struct _writer *writer) {}

static void json_file_preamble(
    DICM_UNUSED struct _writer *writer,
    DICM_UNUSED const struct _dicm_filepreamble *fp) {}

static void json_prefix(DICM_UNUSED struct _writer *writer,
                        DICM_UNUSED const struct _dicm_prefix *prefix) {}

static void json_filemetaelement(
    DICM_UNUSED struct _writer *writer,
    DICM_UNUSED const struct _filemetaelement *fme) {}

static void json_item(DICM_UNUSED struct _writer *writer,
                      DICM_UNUSED const struct _dataelement *de) {
  print_separator();
  printf("{\n");
  separator = NULL;
}

static void json_bot(DICM_UNUSED struct _writer *writer,
                     DICM_UNUSED const struct _dataelement *de) {}

static void json_fragment(DICM_UNUSED struct _writer *writer,
                          DICM_UNUSED const struct _dataelement *de) {}

static void json_end_item(DICM_UNUSED struct _writer *writer,
                          DICM_UNUSED const struct _dataelement *de) {
  printf("}\n");
}

static void json_end_sq(DICM_UNUSED struct _writer *writer,
                        DICM_UNUSED const struct _dataelement *de) {
  printf("]\n");
  printf("}\n");
  separator = ",\n";
}

static void json_end_frags(DICM_UNUSED struct _writer *writer,
                           DICM_UNUSED const struct _dataelement *de) {
  printf("]\n");
}

static void json_sequenceofitems(DICM_UNUSED struct _writer *writer,
                                 DICM_UNUSED const struct _dataelement *de) {
  print_separator();
  printf("\"%04x%04x\": {\n \"vr\": \"%.2s\",\n \"Value\": [\n",
         (unsigned int)get_group(de->tag), (unsigned int)get_element(de->tag),
         get_vr(de->vr));
  separator = NULL;
}

static void json_sequenceoffragments(
    DICM_UNUSED struct _writer *writer,
    DICM_UNUSED const struct _dataelement *de) {
  //  printf("[\n");
  print_separator();
  printf("\"%04x%04x\": {\n \"vr\": \"%.2s\",\n \"Value\": [\n",
         (unsigned int)get_group(de->tag), (unsigned int)get_element(de->tag),
         get_vr(de->vr));
  separator = NULL;
}

static void json_dataelement(DICM_UNUSED struct _writer *writer,
                             DICM_UNUSED const struct _dataelement *de) {
  print_separator();
  const char value[] = "123";
  printf("\"%04x%04x\": {\n \"vr\": \"%.2s\",\n \"Value\": [%s]\n}",
         (unsigned int)get_group(de->tag), (unsigned int)get_element(de->tag),
         get_vr(de->vr), value);

  separator = ",\n";
}

const struct _writer_ops json_writer = {
    // FMI
    .write_start_fmi = json_start_fmi,
    .write_file_preamble = json_file_preamble,      // OPT
    .write_prefix = json_prefix,                    // OPT
    .write_fmi_gl = json_fmi_gl,                    // OPT
    .write_filemetaelement = json_filemetaelement,  // OPT
    .write_end_fmi = json_end_fmi,

    // DataSet:
    // DataElement
    .write_dataelement = json_dataelement,
    // Group
    .write_group_gl = json_group_gl,    // OPT
    .write_end_group = json_end_group,  // OPT
    // SQ
    .write_sequenceofitems = json_sequenceofitems,
    .write_end_sq = json_end_sq,  // (OPT)
    // Item
    .write_item = json_item,
    .write_end_item = json_end_item,  // (OPT)
    // SQ of Frags
    .write_sequenceoffragments = json_sequenceoffragments,
    .write_bot = json_bot,
    .write_fragment = json_fragment,
    .write_end_frags = json_end_frags,
};

#endif
