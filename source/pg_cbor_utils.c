
#include "pg_cbor.h"

#define ENDIAN_IS_NETWORK 0

#if ENDIAN_IS_NETWORK

inline uint16_t bswap16(uint16_t x) { return x; }
inline uint32_t bswap32(uint32_t x) { return x; }
inline uint64_t bswap64(uint64_t x) { return x; }

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
inline uint16_t bswap16(uint16_t x) { return __builtin_bswap16(x); }
# else
inline uint16_t bswap16(uint16_t x) { return __builtin_bswap32((x) << 16; }
# endif
inline uint32_t bswap32(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t bswap64(uint64_t x) { return __builtin_bswap64(x); }

//  Linux systems provide the byteswap.h header, with
#elif defined(__linux__)
//  don't check for obsolete forms defined(linux) and defined(__linux) on the theory that
//  compilers that predefine only these are so old that byteswap.h probably isn't present.
# include <byteswap.h>

inline uint16_t bswap16(uint16_t x) { return bswap_16(x); }
inline uint32_t bswap32(uint32_t x) { return bswap_32(x); }
inline uint64_t bswap64(uint64_t x) { return bswap_64(x); }

#elif defined(_MSC_VER)
//  Microsoft documents these as being compatible since Windows 95 and specificly
//  lists runtime library support since Visual Studio 2003 (aka 7.1).
# include <cstdlib>
inline uint16_t bswap16(uint16_t x) { return _byteswap_ushort(x); }
inline uint32_t bswap32(uint32_t x) { return _byteswap_ulong(x); }
inline uint64_t bswap64(uint64_t x) { return _byteswap_uint64(x); }
#else

inline uint16_t bswap16(uint16_t x) {
	return (x & 0xFF) << 8 | ((x >> 8) & 0xFF);
}

inline uint32_t bswap32(uint32_t x) {
	return x & 0xFF << 24
		| (x >> 8 & 0xFF) << 16
		| (x >> 16 & 0xFF) << 8
		| (x >> 24 & 0xFF);
}
inline uint64_t bswap64(uint64_t x) {
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

bool data_is_cbor(const uint8_t *data, size_t size) {
	if (size > 3 && data[0] == 0xd9 && data[1] == 0xd9 && data[2] == 0xf7) {
		return true;
	}
	return false;
}


uint8_t CborDataReadUnsigned(CborData *data) {
	uint8_t ret = 0;
	if (data->size > 0) {
		ret = *data->ptr;
		data->ptr ++;
		data->size --;
	}
	return ret;
}

uint16_t CborDataReadUnsigned16(CborData *data) {
	uint16_t ret = 0;
	if (data->size > 0) {
		uint16_t ret = *(uint16_t *)data->ptr;
		data->ptr ++;
		data->size --;
	}
	return bswap16(ret);
}

uint32_t CborDataReadUnsigned32(CborData *data) {
	uint32_t ret = 0;
	if (data->size > 0) {
		uint32_t ret = *(uint32_t *)data->ptr;
		data->ptr ++;
		data->size --;
	}
	return bswap32(ret);
}

uint64_t CborDataReadUnsigned64(CborData *data) {
	uint64_t ret = 0;
	if (data->size > 0) {
		uint64_t ret = *(uint64_t *)data->ptr;
		data->ptr ++;
		data->size --;
	}
	return bswap64(ret);
}

uint64_t CborDataReadUnsignedValue(CborData *data, uint8_t type) {
	if (type < CborFlagsMaxAdditionalNumber) {
		return type;
	} else if (type == CborFlagsAdditionalNumber8Bit) {
		return CborDataReadUnsigned();
	} else if (type == CborFlagsAdditionalNumber16Bit) {
		return CborDataReadUnsigned16(data);
	} else if (type == CborFlagsAdditionalNumber32Bit) {
		return CborDataReadUnsigned32(data);
	} else if (type == CborFlagsAdditionalNumber64Bit) {
		return CborDataReadUnsigned64(data);
	} else {
		return 0;
	}
}

CborIterator *CborIteratorInit(const uint8_t *data, size_t size) {
	CborIterator *ret;

	if (data_is_cbor(data, size)) { // prefixed block
		ret = palloc0(size);

		ret->container.ptr = data + 3;
		ret->container.size = size - 3;

		ret->current.ptr = ret->container.ptr;
		ret->current.size = ret->container.size;
	} else if (size > 0) { // non-prefixed
		ret = palloc0(size);

		ret->container.ptr = data;
		ret->container.size = size;

		ret->current.ptr = data;
		ret->current.size = size;
	} else {
		return NULL;
	}

	uint8_t type = *(ret->current.ptr);
	CborMajorTypeEncoded majorType = type & CborFlagsMajorTypeMaskEncoded;
	type = type & CborFlagsAdditionalInfoMask;

	switch(majorType) {
	case CborMajorTypeEncodedUnsigned:
	case CborMajorTypeEncodedNegative:
	case CborMajorTypeEncodedByteString:
	case CborMajorTypeEncodedCharString:
	case CborMajorTypeEncodedSimple:
	case CborMajorTypeEncodedTag:
		ret->state = CborIterStateValue;
		break;
	case CborMajorTypeEncodedArray:
		ret->state = CborIterStateArray;

		break;
	case CborMajorTypeEncodedMap:
		ret->state = CborIterStateObject;

		break;
	}

	return ret;
}

CborIteratorToken CborIteratorNext(CborIterator **it, CborData *value, bool skipNested) {

	uint8_t type = ;
	CborMajorTypeEncoded majorType = ;

	if (!r.empty()) {
		type = r.readUnsigned();
		majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
		type = type & toInt(Flags::AdditionalInfoMask);
		decode(majorType, type, ret);
	}
}
