// Portability shim so the signal-processing core compiles both for the AVR
// target and for the host (PlatformIO `native` environment, unit tests).
//
// On AVR, constant tables live in flash (PROGMEM) and must be read through
// the pgm_read_* / memcpy_P accessors. On the host those collapse to plain
// memory reads.
#pragma once

#include <stdint.h>
#include <string.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef memcpy_P
#define memcpy_P(dst, src, n) memcpy((dst), (src), (n))
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))
#endif
#endif
