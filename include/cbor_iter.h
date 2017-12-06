
#ifndef INCLUDE_CBOR_ITER_H_
#define INCLUDE_CBOR_ITER_H_

#include "cbor_data.h"

#define CBOR_STACK_DEFAULT_SIZE 8

typedef enum {
	CborIteratorTokenDone,
	CborIteratorTokenKey,
	CborIteratorTokenValue,
	CborIteratorTokenBeginArray,
	CborIteratorTokenEndArray,
	CborIteratorTokenBeginObject,
	CborIteratorTokenEndObject,
	CborIteratorTokenBeginByteStrings,
	CborIteratorTokenEndByteStrings,
	CborIteratorTokenBeginCharStrings,
	CborIteratorTokenEndCharStrings,
} CborIteratorToken;

typedef enum {
	CborStackTypeNone,
	CborStackTypeArray,
	CborStackTypeObject,
	CborStackTypeCharString,
	CborStackTypeByteString,
} CborStackType;

typedef enum {
	CborTypeUnsigned,
	CborTypeNegative,
	CborTypeByteString,
	CborTypeCharString,
	CborTypeArray,
	CborTypeMap,
	CborTypeTag,
	CborTypeSimple,
	CborTypeFloat,
	CborTypeTrue,
	CborTypeFalse,
	CborTypeNull,
	CborTypeUndefined,
	CborTypeUnknown,
} CborType;

struct CborIteratorStackValue {
	CborStackType type;
	uint32_t position;
	uint32_t count;
};

typedef struct CborIteratorContext {
	/* Container being iterated */
	CborData current;

	uint8_t type;
	uint8_t info;
	uint32_t objectSize;

	uint32_t stackSize;
	uint32_t stackCapacity;
	struct CborIteratorStackValue *currentStack;
	struct CborIteratorStackValue *stackHead;

	struct CborIteratorStackValue *extendedStack; // palloc'ed stack
	struct CborIteratorStackValue defaultStack[CBOR_STACK_DEFAULT_SIZE]; // preallocated stack
} CborIteratorContext;

bool CborIteratorInit(CborIteratorContext *, const uint8_t *, size_t);
void CborIteratorFinalize(CborIteratorContext *);
void CborIteratorReset(CborIteratorContext *);

CborIteratorToken CborIteratorNext(CborIteratorContext *);

CborType CborIteratorGetType(const CborIteratorContext *);
CborStackType CborIteratorGetContainerType(const CborIteratorContext *);

uint32_t CborIteratorGetContainerSize(const CborIteratorContext *);
uint32_t CborIteratorGetContainerPosition(const CborIteratorContext *);
uint32_t CborIteratorGetObjectSize(const CborIteratorContext *);

int64_t CborIteratorGetInteger(const CborIteratorContext *);
uint64_t CborIteratorGetUnsigned(const CborIteratorContext *);
double CborIteratorGetFloat(const CborIteratorContext *);
const char *CborIteratorGetCharPtr(const CborIteratorContext *);
const uint8_t *CborIteratorGetBytePtr(const CborIteratorContext *);

#endif /* INCLUDE_CBOR_ITER_H_ */
