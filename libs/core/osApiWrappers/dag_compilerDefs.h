#pragma once

#if !defined(_MSC_VER) && !defined(__forceinline)
#define __forceinline inline __attribute__((always_inline))
#endif

#if !defined(_MSC_VER)
#define __cdecl
#define __stdcall
#endif

#if !defined(_MSC_VER)
#include <stdio.h>
#include <string.h>

#define _vsnprintf vsnprintf

inline int strncpy_s(char *dest, size_t destsz, const char *src, size_t count) {
  strncpy(dest, src, count);
  if (count < destsz) {
    dest[count] = '\0';
  } else {
    dest[destsz - 1] = '\0';
  }
  return 0;
}
#endif
