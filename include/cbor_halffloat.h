
#ifndef INCLUDE_CBOR_HALFFLOAT_H_
#define INCLUDE_CBOR_HALFFLOAT_H_

#include <stdint.h>

// see https://en.wikipedia.org/wiki/Half_precision_floating-point_format

static inline uint16_t CborHalfFloatGetNaN() { return (uint16_t)0x7e00; }
static inline uint16_t CborHalfFloatGetPosInf() { return (uint16_t)(31 << 10); }
static inline uint16_t CborHalfFloatGetNegInf() { return (uint16_t)(63 << 10); }

float CborHalfFloatDecode(uint16_t half);
uint16_t CborHalfFloatEncode(float val);

#endif /* INCLUDE_CBOR_HALFFLOAT_H_ */
