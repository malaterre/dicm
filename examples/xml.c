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
#include "dicm-public.h"

#include "dicm-io.h"
#include "dicm-writer.h"

#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _xml {
  struct dicm_writer writer;
  /* data */
  const char *separator;
  bool pretty;
  int indent_level;
  dicm_vr_t vr;
  dicm_vl_t vl;
};

/* object */
static DICM_CHECK_RETURN int _xml_destroy(void *self_) DICM_NONNULL;

/* writer */
static DICM_CHECK_RETURN int _xml_write_start_attribute(
    void *self, const struct dicm_attribute *da) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_value(void *self, const void *buf,
                                              size_t s) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_start_fragment(void *self, int frag_num)
    DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_end_fragment(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_start_item(void *self,
                                                   int item_num) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_end_item(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_start_sequence(
    void *self, const struct dicm_attribute *da) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_end_sequence(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_start_model(
    void *self, const char *encoding) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_end_model(void *self) DICM_NONNULL;

static struct writer_vtable const g_vtable =
    {/* object interface */
     .object = {.fp_destroy = _xml_destroy},
     /* writer interface */
     .writer = {
         .fp_write_start_attribute = _xml_write_start_attribute,
         .fp_write_value = _xml_write_value,
         .fp_write_start_fragment = _xml_write_start_fragment,
         .fp_write_end_fragment = _xml_write_end_fragment,
         .fp_write_start_item = _xml_write_start_item,
         .fp_write_end_item = _xml_write_end_item,
         .fp_write_start_sequence = _xml_write_start_sequence,
         .fp_write_end_sequence = _xml_write_end_sequence,
         .fp_write_start_model = _xml_write_start_model,
         .fp_write_end_model = _xml_write_end_model,
     }};

int dicm_xml_writer_create(struct dicm_writer **pself, struct dicm_io *dst) {
  struct _xml *self = (struct _xml *)malloc(sizeof(*self));
  if (self) {
    *pself = &self->writer;
    self->writer.vtable = &g_vtable;
    self->writer.dst = dst;
    self->separator = NULL;
    self->pretty = true;
    self->indent_level = 0;
    self->vr = VR_NONE;
    return 0;
  }
  return 1;
}

/* object */
int _xml_destroy(void *self_) {
  struct _xml *self = (struct _xml *)self_;
  free(self);
  return 0;
}

static void print_with_indent(int indent, const char *string) {
  printf("%*s%s", indent, "", string);
}

static void print_indent(struct _xml *self) {
  if (self->pretty) {
    const int level = 2 * self->indent_level;
    print_with_indent(level, "");
  }
}
static void print_eol(struct _xml *self) {
  struct dicm_io *dst = self->writer.dst;
  if (self->pretty) {
    const char eol[] = "\n";
    io_ssize err = dicm_io_write(dst, eol, strlen(eol));
    assert(err == 1);
  }
}

static int _xml_write_line(struct _xml *self, const char *line) {
  struct dicm_io *dst = self->writer.dst;
  io_ssize err = dicm_io_write(dst, line, strlen(line));
  if (err != (io_ssize)strlen(line)) return 1;
  print_eol(self);
  return 0;
}

static void print_separator(struct _xml *self) {
  assert(0);
  if (self->separator) {
    if (self->pretty) printf(self->separator);
  }
}

static void print_simple(struct _xml *self, const char *str, size_t len) {
  assert(0);
  assert(len);
  printf("\"%.*s\"", len, str);
}

static void print_no_whitespace(struct _xml *self, const char *str,
                                size_t len) {
  _xml_write_line(self, "TODO2");
}

static void print_with_separator(struct _xml *self, const char *str, size_t len,
                                 bool quotes) {
  assert(len);

  if (str[len - 1] == 0x0) len--;
  if (*str == ' ') {
    ++str;
    --len;
  }
  if (str[len - 1] == ' ') len--;

  char buffer0[512];
  char buffer[512];
  char *cur = buffer;
  int index = 1;
  for (const char *pos = str; pos != str + len; ++pos) {
    if (*pos == '\\') {
      *cur = 0;
      snprintf(buffer0, sizeof buffer0, "<Value number=\"%d\">%s</Value>",
               index, buffer);
      ++index;
      _xml_write_line(self, buffer0);
      cur = buffer;
    } else {
      *cur++ = *pos;
    }
  }
  if (cur != buffer) {
    *cur = 0;
    snprintf(buffer0, sizeof buffer0, "<Value number=\"%d\">%s</Value>", index,
             buffer);
    _xml_write_line(self, buffer0);
  }
}

static void print_person_name(struct _xml *self, const char *str, size_t len) {
  _xml_write_line(self, "TODO1");
}

static void print_signed_short(struct _xml *self, const void *buf, size_t len) {
  const int16_t *values = buf;
  const size_t nvalues = len / sizeof(int16_t);
  char buffer[512];
  for (size_t n = 0; n < nvalues; ++n) {
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%hd</Value>", n + 1,
             values[n]);
    _xml_write_line(self, buffer);
  }
}

static void print_unsigned_short(struct _xml *self, const void *buf,
                                 size_t len) {
  const uint16_t *values = buf;
  const size_t nvalues = len / sizeof(uint16_t);
  char buffer[512];
  for (size_t n = 0; n < nvalues; ++n) {
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%hu</Value>", n + 1,
             values[n]);
    _xml_write_line(self, buffer);
  }
}

static void print_signed_long(struct _xml *self, const void *buf, size_t len) {
  const int32_t *values = buf;
  const size_t nvalues = len / sizeof(int32_t);
  char buffer[512];
  for (size_t n = 0; n < nvalues; ++n) {
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%d</Value>", n + 1,
             values[n]);
    _xml_write_line(self, buffer);
  }
}

static void print_signed_very_long(struct _xml *self, const void *buf,
                                   size_t len) {
  const int64_t *values = buf;
  const size_t nvalues = len / sizeof(int64_t);
  char buffer[512];
  for (size_t n = 0; n < nvalues; ++n) {
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%ld</Value>", n + 1,
             values[n]);
    _xml_write_line(self, buffer);
  }
}

static void print_unsigned_long(struct _xml *self, const void *buf,
                                size_t len) {
  const uint32_t *values = buf;
  const size_t nvalues = len / sizeof(uint32_t);
  char buffer[512];
  for (size_t n = 0; n < nvalues; ++n) {
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%u</Value>", n + 1,
             values[n]);
    _xml_write_line(self, buffer);
  }
}

static void print_unsigned_very_long(struct _xml *self, const void *buf,
                                     size_t len) {
  const uint64_t *values = buf;
  const size_t nvalues = len / sizeof(uint64_t);
  char buffer[512];
  for (size_t n = 0; n < nvalues; ++n) {
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%lu</Value>", n + 1,
             values[n]);
    _xml_write_line(self, buffer);
  }
}

// https://stackoverflow.com/questions/16839658/printf-width-specifier-to-maintain-precision-of-floating-point-value
static void print_float(struct _xml *self, const void *buf, size_t len) {
  const float *values = buf;
  const size_t nvalues = len / sizeof(float);
  char buffer[512];
  for (size_t n = 0; n < nvalues; ++n) {
    //    printf("%.*g", FLT_DECIMAL_DIG, values[n]);
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%.*g</Value>", n + 1,
             FLT_DECIMAL_DIG, values[n]);
    _xml_write_line(self, buffer);
  }
}

static void print_double(struct _xml *self, const void *buf, size_t len) {
  const double *values = buf;
  const size_t nvalues = len / sizeof(double);
  char buffer[512];
  for (size_t n = 0; n < nvalues; ++n) {
    //    printf("%.*g", DBL_DECIMAL_DIG, values[n]);
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%.*g</Value>", n + 1,
             DBL_DECIMAL_DIG, values[n]);
    _xml_write_line(self, buffer);
  }
}

static void print_at(struct _xml *self, const void *buf, size_t len) {}

static inline size_t base64_encoded_size(size_t inlen) {
  // inlen is uint32_t anyway:
  const size_t outlen = (inlen + 2) / 3;
  return outlen * 4;
}

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static void base64_print(const unsigned char *in, size_t len) {
  assert(len);
  unsigned char out[4];
  for (size_t i = 0, j = 0; i < len; i += 3, j += 4) {
    size_t v = in[i];
    v = i + 1 < len ? v << 8 | in[i + 1] : v << 8;
    v = i + 2 < len ? v << 8 | in[i + 2] : v << 8;

    out[0] = base64_chars[(v >> 18) & 0x3F];
    out[1] = base64_chars[(v >> 12) & 0x3F];
    out[2] = i + 1 < len ? base64_chars[(v >> 6) & 0x3F] : '=';
    out[3] = i + 2 < len ? base64_chars[v & 0x3F] : '=';

    printf("%.*s", sizeof out, out);
  }
}

static void print_inline_binary(struct _xml *self, const void *buf, size_t len,
                                bool is_undefined_length) {
  if (!is_undefined_length) {
    const unsigned char *values = buf;
    const size_t nvalues = len / sizeof(unsigned char);
    base64_print(values, nvalues);
  }
}

/* writer */
int _xml_write_start_attribute(void *self_, const struct dicm_attribute *da) {
  struct _xml *self = (struct _xml *)self_;
  struct dicm_io *dst = self->writer.dst;

  const dicm_vr_t vr = da->vr;
  const dicm_vl_t vl = da->vl;

  char buffer[512];
  snprintf(buffer, sizeof buffer,
           "<DicomAttribute tag=\"%04X%04X\" vr=\"%.2s\" keyword=\"\">",
           (unsigned int)dicm_tag_get_group(da->tag),
           (unsigned int)dicm_tag_get_element(da->tag),
           dicm_vr_get_string(da->vr));
  if (_xml_write_line(self, buffer)) return 1;
  self->separator = ",\n";
  // store vr/vl so that we know what to do in end_attribute call
  self->vr = vr;
  self->vl = vl;

  return 0;
}

#if 0
int _xml_write_end_attribute(void *self_) {
  struct _xml *self = (struct _xml *)self_;
  const char line[] = "</DicomAttribute>";
  if (_xml_write_line(self, line)) return 1;
  self->indent_level--;
  return 0;
}
#endif

int _xml_write_value(void *self_, const void *buf, size_t s) {
  struct _xml *self = (struct _xml *)self_;
  if (!s) {
    return 0;
  }
  bool is_undefined_length = dicm_vl_is_undefined(self->vl);
  const dicm_vr_t vr = self->vr;
  const char *debug = dicm_vr_get_string(vr);
  switch (vr) {
    case VR_AE:
    case VR_AS:
    case VR_CS:
    case VR_DA:
    case VR_DT:
    case VR_LO:
    case VR_SH:
    case VR_TM:
    case VR_UC:
    case VR_UI:
      assert(!is_undefined_length);
      print_with_separator(self, buf, s, true);
      break;
    case VR_DS:
    case VR_IS:
      assert(!is_undefined_length);
      print_with_separator(self, buf, s, false);
      break;
    case VR_LT: /* cannot be VM 1-N */
    case VR_ST:
    case VR_UR:
    case VR_UT:
      assert(!is_undefined_length);
      print_no_whitespace(self, buf, s);
      break;
    case VR_PN:
      assert(!is_undefined_length);
      print_person_name(self, buf, s);
      break;
      /* binary */
    case VR_OB:
    case VR_OD:
    case VR_OF:
    case VR_OL:
    case VR_OV:
    case VR_OW:
      print_inline_binary(self, buf, s, is_undefined_length);
      break;
    case VR_SL:
      assert(!is_undefined_length);
      print_signed_long(self, buf, s);
      break;
    case VR_SV:
      assert(!is_undefined_length);
      print_signed_very_long(self, buf, s);
      break;
    case VR_SS:
      assert(!is_undefined_length);
      print_signed_short(self, buf, s);
      break;
    case VR_UL:
      assert(!is_undefined_length);
      print_unsigned_long(self, buf, s);
      break;
    case VR_US:
      assert(!is_undefined_length);
      print_unsigned_short(self, buf, s);
      break;
    case VR_UV:
      assert(!is_undefined_length);
      print_unsigned_very_long(self, buf, s);
      break;
    case VR_FL:
      assert(!is_undefined_length);
      print_float(self, buf, s);
      break;
    case VR_FD:
      assert(!is_undefined_length);
      print_double(self, buf, s);
      break;
    case VR_AT:
      assert(!is_undefined_length);
      print_at(self, buf, s);
      break;
    case VR_UN:
      assert(!is_undefined_length);
      assert(0);
      break;
    case VR_SQ:
      assert(0);
      break;
    case VR_NONE:
      // FIXME: should only happen for Fragment ?
      // print_inline_binary(self, buf, s, is_undefined_length);
      break;
    default:
      assert(0);
  }

  return 0;
}
int _xml_write_start_fragment(void *self, int frag_num) { return 0; }
int _xml_write_end_fragment(void *self) { return 0; }
int _xml_write_start_item(void *self_, int item_num) {
  struct _xml *self = (struct _xml *)self_;

  const char item[] = "<Item number=\"%d\">";
  char buffer[512];
  snprintf(buffer, sizeof buffer, item, item_num);
  if (_xml_write_line(self, buffer)) return 1;

  return 0;
}
int _xml_write_end_item(void *self_) {
  struct _xml *self = (struct _xml *)self_;

  const char item[] = "</Item>";
  if (_xml_write_line(self, item)) return 1;

  return 0;
}
int _xml_write_start_sequence(void *self_, const struct dicm_attribute *da) {
  return 0;
}
int _xml_write_end_sequence(void *self_) { return 0; }
int _xml_write_start_model(void *self_, const char *encoding) {
  struct _xml *self = (struct _xml *)self_;
  struct dicm_io *dst = self->writer.dst;
  const char header[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  if (_xml_write_line(self, header)) return 1;

  const char line[] = "<NativeDicomModel xml:space=\"preserve\">";
  if (_xml_write_line(self, line)) return 1;

  return 0;
}
int _xml_write_end_model(void *self_) {
  struct _xml *self = (struct _xml *)self_;
  const char line[] = "</NativeDicomModel>";
  if (_xml_write_line(self, line)) return 1;
  return 0;
}
