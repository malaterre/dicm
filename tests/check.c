#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main() {
  {
    const char buffer[] = {0xE0, 0x7F, 0x10, 0x00, 0x4F, 0x42,
                           0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    if (sizeof buffer != 12) return 1;
    struct _de {
      uint16_t group;
      uint16_t element;
      uint8_t vr[2];
      uint16_t reserved;  // fixme, hide me
      uint32_t length;
    } de;
    fprintf(stdout, "%d\n", sizeof de);
    //  if( sizeof de != 12 ) return 1;
    memcpy(&de, buffer, sizeof buffer);
    fprintf(stdout, "%04x\n", de.group);
    fprintf(stdout, "%04x\n", de.element);
    fprintf(stdout, "%c%c\n", de.vr[0], de.vr[1]);
    //  fprintf(stdout, "%04x\n", de.reserved);
    fprintf(stdout, "%x\n", de.length);
  }

  {
    const char buffer[] = {0x08, 0x00, 0x22, 0x00, 0x44, 0x41, 0x08, 0x00};
    if (sizeof buffer != 8) return 1;
    struct _de {
      uint16_t group;
      uint16_t element;
      uint8_t vr[2];
      uint16_t length;
    } de;
    fprintf(stdout, "%d\n", sizeof de);
    memcpy(&de, buffer, sizeof buffer);
    fprintf(stdout, "%04x\n", de.group);
    fprintf(stdout, "%04x\n", de.element);
    fprintf(stdout, "%c%c\n", de.vr[0], de.vr[1]);
    fprintf(stdout, "%x\n", de.length);
  }

  return 0;
}
