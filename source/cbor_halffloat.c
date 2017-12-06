
#include <math.h>
#include <string.h>
#include "cbor_halffloat.h"

float CborHalfFloatDecode(uint16_t half) {
	uint16_t exp = (half >> 10) & 0x1f;
	uint16_t mant = half & 0x3ff;
	float val;

	if (exp == 0) {
		val = ldexp(mant, -24);
	} else if (exp != 31) {
		val = ldexp(mant + 1024, exp - 25);
	} else {
		val = mant == 0 ? INFINITY : NAN;
	}

	return (half & 0x8000) ? -val : val;
}

uint16_t CborHalfFloatEncode(float val) {
	uint32_t u32_i, bits, m, e;
	memcpy(&u32_i, &val, sizeof(uint32_t));

	bits = (u32_i >> 16) & 0x8000; /* Get the sign */
	m = (u32_i >> 12) & 0x07ff; /* Keep one extra bit for rounding */
	e = (u32_i >> 23) & 0xff; /* Using int is faster here */

	/* If zero, or denormal, or exponent underflows too much for a denormal
	 * half, return signed zero. */
	if (e < 103) {
		return bits;
	}

	/* If NaN, return NaN. If Inf or exponent overflow, return Inf. */
	if (e > 142) {
		bits |= 0x7c00u;
		/* If exponent was 0xff and one mantissa bit was set, it means NaN,
		 * not Inf, so make sure we set one mantissa bit too. */
		bits |= (e == 255) && (u32_i & 0x007fffffu);
		return bits;
	}

	/* If exponent underflows but not too much, return a denormal */
	if (e < 113) {
		m |= 0x0800u;
		/* Extra rounding may overflow and set mantissa to 0 and exponent
		 * to 1, which is OK. */
		bits |= (m >> (114 - e)) + ((m >> (113 - e)) & 1);
		return bits;
	}

	bits |= ((e - 112) << 10) | (m >> 1);
	/* Extra rounding. An overflow will set mantissa to 0 and increment
	 * the exponent, which is OK. */
	bits += m & 1;
	return bits;
}
