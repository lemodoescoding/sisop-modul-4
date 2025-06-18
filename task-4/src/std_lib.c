#include "std_type.h"
#include "std_lib.h"

int div(int a, int b) {
  int dividend = 0;

  while (a >= b) {
    a = a - b;
    dividend = dividend + 1;
  }

  return dividend;
}

int mod(int a, int b) {
  while (a >= b) {
    a = a - b;
  }

  return a;
}

void memcpy(byte *src, byte *dst, unsigned int size) {
  unsigned int i;
  for (i = 0; i < size; i++) {
    dst[i] = src[i];
  }
}

unsigned int strlen(char *str) {
  unsigned int i = 0;

  while (str[i] != '\0') {
    i++;
  }

  return i;
}

bool strcmp(char *str1, char *str2) {
  unsigned int i = 0;

  while (str1[i] != '\0' && str2[i] != '\0') {
    if (str1[i] != str2[i])
      return 0;

    i++;
  }

  return str1[i] == '\0' && str2[i] == '\0';
}

bool strncmp(char *str1, char *str2, unsigned int check) {
  unsigned int i;
  for (i = 0; i < check; i++) {
    if (str1[i] == '\0' || str2[i] == '\0')
      return 0;

    if (str1[i] != str2[i])
      return 0;
  }

  return 1;
}

void strcpy(char *src, char *dst) {
  unsigned int i = 0;

  while (src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }

  dst[i] = '\0';
}

void clear(byte *buf, unsigned int size) {
  unsigned int i;
  for (i = 0; i < size; i++) {
    buf[i] = 0x00;
  }
}
