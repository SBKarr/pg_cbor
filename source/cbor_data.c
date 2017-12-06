
#include <string.h>
#include "cbor_data.h"
#include "cbor_typeinfo.h"
#include "cbor_halffloat.h"

#define ENDIAN_IS_NETWORK 0

#if ENDIAN_IS_NETWORK

static inline uint16_t bswap16(uint16_t x) { return x; }
static inline uint32_t bswap32(uint32_t x) { return x; }
static inline uint64_t bswap64(uint64_t x) { return x; }

#else

#ifndef __has_builtin         // Optional of course
  #define __has_builtin(x) 0  // Compatibility with non-clang compilers
#endif

//  Adapted code from BOOST_ENDIAN_INTRINSICS
//  GCC and Clang recent versions provide intrinsic byte swaps via builtins
#if (defined(__clang__) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64)) \
  || (defined(__GNUC__ ) && \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))

// prior to 4.8, gcc did not provide __builtin_bswap16 on some platforms so we emulate it
// see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=52624
// Clang has a similar problem, but their feature test macros make it easier to detect

# if (defined(__clang__) && __has_builtin(__builtin_bswap16)) \
  || (defined(__GNUC__) &&(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
static inline uint16_t bswap16(uint16_t x) { return __builtin_bswap16(x); }
# else
static inline uint16_t bswap16(uint16_t x) { return __builtin_bswap32((x) << 16; }
# endif
static inline uint32_t bswap32(uint32_t x) { return __builtin_bswap32(x); }
static inline uint64_t bswap64(uint64_t x) { return __builtin_bswap64(x); }

//  Linux systems provide the byteswap.h header, with
#elif defined(__linux__)
//  don't check for obsolete forms defined(linux) and defined(__linux) on the theory that
//  compilers that predefine only these are so old that byteswap.h probably isn't present.
# include <byteswap.h>

static inline uint16_t bswap16(uint16_t x) { return bswap_16(x); }
static inline uint32_t bswap32(uint32_t x) { return bswap_32(x); }
static inline uint64_t bswap64(uint64_t x) { return bswap_64(x); }

#elif defined(_MSC_VER)
//  Microsoft documents these as being compatible since Windows 95 and specificly
//  lists runtime library support since Visual Studio 2003 (aka 7.1).
# include <cstdlib>
static inline uint16_t bswap16(uint16_t x) { return _byteswap_ushort(x); }
static inline uint32_t bswap32(uint32_t x) { return _byteswap_ulong(x); }
static inline uint64_t bswap64(uint64_t x) { return _byteswap_uint64(x); }
#else

static inline uint16_t bswap16(uint16_t x) {
	return (x & 0xFF) << 8 | ((x >> 8) & 0xFF);
}

static inline uint32_t bswap32(uint32_t x) {
	return x & 0xFF << 24
		| (x >> 8 & 0xFF) << 16
		| (x >> 16 & 0xFF) << 8
		| (x >> 24 & 0xFF);
}
static inline uint64_t bswap64(uint64_t x) {
	return x & 0xFF << 56
		| (x >> 8 & 0xFF) << 48
		| (x >> 16 & 0xFF) << 40
		| (x >> 24 & 0xFF) << 32
		| (x >> 32 & 0xFF) << 24
		| (x >> 40 & 0xFF) << 16
		| (x >> 48 & 0xFF) << 8
		| (x >> 56 & 0xFF);
}

#endif

#endif

bool data_is_cbor(const uint8_t *data, uint32_t size) {
	if (size > 3 && data[0] == 0xd9 && data[1] == 0xd9 && data[2] == 0xf7) {
		return true;
	}
	return false;
}

uint8_t CborDataGetUnsigned(const CborData *data) {
	return *data->ptr;
}

uint16_t CborDataGetUnsigned16(const CborData *data) {
	uint16_t ret = 0;
	memcpy(&ret, data->ptr, sizeof(uint16_t));
	return bswap16(ret);
}

uint32_t CborDataGetUnsigned32(const CborData *data) {
	uint32_t ret = 0;
	memcpy(&ret, data->ptr, sizeof(uint32_t));
	return bswap32(ret);
}

uint64_t CborDataGetUnsigned64(const CborData *data) {
	uint64_t ret = 0;
	memcpy(&ret, data->ptr, sizeof(uint64_t));
	return bswap64(ret);
}

float CborDataGetFloat16(const CborData *data) {
	uint16_t value = CborDataGetUnsigned16(data);
	return CborHalfFloatDecode(value);
}

float CborDataGetFloat32(const CborData *data) {
	float ret = 0.0f;
	uint32_t value = CborDataGetUnsigned32(data);
	memcpy(&ret, &value, sizeof(float));
	return ret;
}

double CborDataGetFloat64(const CborData *data) {
	double ret = 0.0f;
	uint64_t value = CborDataGetUnsigned64(data);
	memcpy(&ret, &value, sizeof(double));
	return ret;
}

uint64_t CborDataGetUnsignedValue(const CborData *data, uint8_t type) {
	if (type < CborFlagsMaxAdditionalNumber) {
		return type;
	} else if (type == CborFlagsAdditionalNumber8Bit) {
		return CborDataGetUnsigned(data);
	} else if (type == CborFlagsAdditionalNumber16Bit) {
		return CborDataGetUnsigned16(data);
	} else if (type == CborFlagsAdditionalNumber32Bit) {
		return CborDataGetUnsigned32(data);
	} else if (type == CborFlagsAdditionalNumber64Bit) {
		return CborDataGetUnsigned64(data);
	} else {
		return 0;
	}
}

uint64_t CborDataReadUnsignedValue(CborData *data, uint8_t type) {
	uint64_t ret = CborDataGetUnsignedValue(data, type);
	switch (type) {
	case CborFlagsAdditionalNumber8Bit:
		CborDataOffset(data, 1);
		break;
	case CborFlagsAdditionalNumber16Bit:
		CborDataOffset(data, 2);
		break;
	case CborFlagsAdditionalNumber32Bit:
		CborDataOffset(data, 4);
		break;
	case CborFlagsAdditionalNumber64Bit:
		CborDataOffset(data, 8);
		break;
	default: break;
	}
	return ret;
}
