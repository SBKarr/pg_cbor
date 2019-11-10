
#ifndef INCLUDE_CBOR_H_
#define INCLUDE_CBOR_H_

#include "cbor_alloc.h"
#include "cbor_typeinfo.h"
#include "cbor_iter.h"

typedef void (*CborWriterPlain) (void *, const char *, int);
typedef void (*CborWriterFormat) (void *, const char *, ...);

struct CborWriter {
	CborWriterPlain plain;
	CborWriterFormat format;
	void *ctx;
};

bool CborToString(const struct CborWriter *writer, const uint8_t *data, uint32_t size);

bool CborIteratorValueToString(const struct CborWriter *writer, CborIteratorContext *ctx);

#endif /* INCLUDE_CBOR_H_ */
