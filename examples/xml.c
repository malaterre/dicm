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
#define _XOPEN_SOURCE 700
#include "dicm-public.h"

#include "dicm-io.h"
#include "dicm-writer.h"

#include <assert.h>
#include <float.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _xml {
  struct dicm_writer writer;
  /* data */
  locale_t c_locale;
  bool first_attribute;
  int item_num;
  bool pretty;
  int indent_level;
  dicm_vr_t vr;
  dicm_vl_t vl;
};

/* object */
static DICM_CHECK_RETURN int _xml_destroy(void *self_) DICM_NONNULL;

/* writer */
static DICM_CHECK_RETURN int _xml_write_attribute(
    void *self, const struct dicm_attribute *da) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_value_length(void *self,
                                                     size_t s) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_value(void *self, const void *buf,
                                              size_t s) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_fragment(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_start_item(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_end_item(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_start_sequence(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_end_sequence(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_start_dataset(
    void *self, const char *encoding) DICM_NONNULL;
static DICM_CHECK_RETURN int _xml_write_end_dataset(void *self) DICM_NONNULL;

static struct writer_vtable const g_vtable =
    {/* object interface */
     .object = {.fp_destroy = _xml_destroy},
     /* writer interface */
     .writer = {
         .fp_write_attribute = _xml_write_attribute,
         .fp_write_value = _xml_write_value,
         .fp_write_value_length = _xml_write_value_length,
         .fp_write_fragment = _xml_write_fragment,
         .fp_write_start_item = _xml_write_start_item,
         .fp_write_end_item = _xml_write_end_item,
         .fp_write_start_sequence = _xml_write_start_sequence,
         .fp_write_end_sequence = _xml_write_end_sequence,
         .fp_write_start_dataset = _xml_write_start_dataset,
         .fp_write_end_dataset = _xml_write_end_dataset,
     }};

int dicm_xml_writer_create(struct dicm_writer **pself, struct dicm_io *dst) {
  struct _xml *self = (struct _xml *)malloc(sizeof(*self));
  if (self) {
    *pself = &self->writer;
    self->writer.vtable = &g_vtable;
    self->writer.dst = dst;
    self->c_locale = newlocale(LC_NUMERIC, "C", NULL);
    self->first_attribute = true;
    self->item_num = 0;
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
  freelocale(self->c_locale);
  free(self);
  return 0;
}

static int print_eol(struct _xml *self) {
  struct dicm_io *dst = self->writer.dst;
  if (self->pretty) {
    const char eol[] = "\n";
    io_ssize err = dicm_io_write(dst, eol, 1);
    if (err != 1) return 1;
  }
  return 0;
}

static int _xml_write_buffer(struct _xml *self, const char *line,
                             const size_t count) {
  struct dicm_io *dst = self->writer.dst;
  const io_ssize err = dicm_io_write(dst, line, count);
  if (err != (io_ssize)count) return 1;
  if (print_eol(self)) return 1;
  return 0;
}

static int _xml_write_line(struct _xml *self, const char *line) {
  return _xml_write_buffer(self, line, strlen(line));
}

static int print_no_whitespace(struct _xml *self, const char *str, size_t len) {
  char buffer[512];
  const int index = 1;
  snprintf(buffer, sizeof buffer, "<Value number=\"%d\">%.*s</Value>", index,
           len, str);
  _xml_write_line(self, buffer);
  return 0;
}

static int print_with_separator(struct _xml *self, const char *str, size_t len,
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
  return 0;
}

static int print_person_name(struct _xml *self, const char *str, size_t len) {
  const char header[] = "<PersonName number=\"1\">";
  const char trailer[] = "</PersonName>";
  const char line1[] = "<Alphabetic>";
  const char line2[] = "</Alphabetic>";

  const char family_name[] = "<FamilyName>NFAO</FamilyName>";

  _xml_write_line(self, header);
  _xml_write_line(self, line1);
  _xml_write_line(self, family_name);

  _xml_write_line(self, line2);
  _xml_write_line(self, trailer);
  return 0;
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
static void print_float(struct _xml *self, const void *buf, const size_t len) {
  const float *values = buf;
  const size_t nvalues = len / sizeof(float);
  char buffer[512];
  locale_t old_locale = uselocale(self->c_locale);
  for (size_t n = 0; n < nvalues; ++n) {
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%.*g</Value>", n + 1,
             FLT_DECIMAL_DIG, values[n]);
    _xml_write_line(self, buffer);
  }
  /* Switch back to original locale. */
  uselocale(old_locale);
}

static void print_double(struct _xml *self, const void *buf, const size_t len) {
  const double *values = buf;
  const size_t nvalues = len / sizeof(double);
  char buffer[512];
  locale_t old_locale = uselocale(self->c_locale);
  for (size_t n = 0; n < nvalues; ++n) {
    snprintf(buffer, sizeof buffer, "<Value number=\"%u\">%.*g</Value>", n + 1,
             DBL_DECIMAL_DIG, values[n]);
    _xml_write_line(self, buffer);
  }
  /* Switch back to original locale. */
  uselocale(old_locale);
}

static void print_at(struct _xml *self, const void *buf, size_t len) {}

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static void base64_print(struct _xml *self, const unsigned char *in,
                         size_t len) {
  assert(len);
  unsigned char out[4];
  char buffer[512];
  for (size_t i = 0, j = 0; i < len; i += 3, j += 4) {
    size_t v = in[i];
    v = i + 1 < len ? v << 8 | in[i + 1] : v << 8;
    v = i + 2 < len ? v << 8 | in[i + 2] : v << 8;

    out[0] = base64_chars[(v >> 18) & 0x3F];
    out[1] = base64_chars[(v >> 12) & 0x3F];
    out[2] = i + 1 < len ? base64_chars[(v >> 6) & 0x3F] : '=';
    out[3] = i + 2 < len ? base64_chars[v & 0x3F] : '=';

    //    printf("%.*s", sizeof out, out);
    snprintf(buffer, sizeof buffer, "%.*s", sizeof out, out);

    _xml_write_line(self, buffer);
  }
}

static void print_inline_binary(struct _xml *self, const void *buf, size_t len,
                                bool is_undefined_length) {
  if (!is_undefined_length) {
    const unsigned char *values = buf;
    const size_t nvalues = len / sizeof(unsigned char);
    base64_print(self, values, nvalues);
  }
}

static int _xml_write_end_attribute(struct _xml *self) {
  if (!self->first_attribute) {
    const char end_att[] = "</DicomAttribute>";
    if (_xml_write_line(self, end_att)) return 1;
  }
  return 0;
}

/* writer */
int _xml_write_attribute(void *self_, const struct dicm_attribute *da) {
  struct _xml *self = (struct _xml *)self_;
  struct dicm_io *dst = self->writer.dst;
  if (_xml_write_end_attribute(self)) return 1;

  const dicm_tag_t tag = da->tag;
  const dicm_vr_t vr = da->vr;
  const dicm_vl_t vl = da->vl;

  const unsigned int group = dicm_tag_get_group(da->tag);
  unsigned int element = dicm_tag_get_element(da->tag);

  const char *privateCreator = "";
  if (dicm_tag_is_private(tag)) {
    privateCreator = " privateCreator=\"libdicm\"";
    element = element & 0x00ff;
  }
  char buffer[512];
  int count =
      snprintf(buffer, sizeof buffer,
               "<DicomAttribute tag=\"%04X%04X\" vr=\"%.2s\" keyword=\"\"%s>",
               group, element, dicm_vr_get_string(da->vr), privateCreator);
  if (_xml_write_line(self, buffer)) return 1;
  // store vr/vl so that we know what to do in write_value call
  self->vr = vr;
  self->vl = vl;
  self->first_attribute = false;

  return 0;
}

int _xml_write_value(void *self_, const void *buf, const size_t s) {
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
int _xml_write_value_length(void *self_, size_t s) { return 0; }
int _xml_write_fragment(void *self) {
  const char buffer[] = "fragment";
  if (_xml_write_line(self, buffer)) return 1;
  return 0;
}
int _xml_write_start_item(void *self_) {
  struct _xml *self = (struct _xml *)self_;
  self->item_num++;
  self->first_attribute = true;

  const char item[] = "<Item number=\"%d\">";
  char buffer[512];
  snprintf(buffer, sizeof buffer, item, self->item_num);
  if (_xml_write_line(self, buffer)) return 1;

  return 0;
}
int _xml_write_end_item(void *self_) {
  struct _xml *self = (struct _xml *)self_;
  if (_xml_write_end_attribute(self)) return 1;

  const char item[] = "</Item>";
  if (_xml_write_line(self, item)) return 1;

  return 0;
}
int _xml_write_start_sequence(void *self_) {
  struct _xml *self = (struct _xml *)self_;
  self->item_num = 0;
  return 0;
}
int _xml_write_end_sequence(void *self_) {
  struct _xml *self = (struct _xml *)self_;
  //  assert (self->first_attribute == false) ;
  if (_xml_write_end_attribute(self)) return 1;
  self->item_num = 0;
  self->first_attribute = true;
  return 0;
}
int _xml_write_start_dataset(void *self_, const char *encoding) {
  struct _xml *self = (struct _xml *)self_;
  struct dicm_io *dst = self->writer.dst;
  const char header[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  if (_xml_write_line(self, header)) return 1;

  const char line[] = "<NativeDicomModel xml:space=\"preserve\">";
  if (_xml_write_line(self, line)) return 1;

  return 0;
}
int _xml_write_end_dataset(void *self_) {
  struct _xml *self = (struct _xml *)self_;
  const char line[] = "</NativeDicomModel>";
  if (_xml_write_line(self, line)) return 1;
  return 0;
}
