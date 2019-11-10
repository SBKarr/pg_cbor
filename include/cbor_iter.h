
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
	const uint8_t *ptr;
};

typedef struct CborIteratorContext {
	/* Container being iterated */
	CborData current;

	uint8_t type;
	uint8_t info;

	bool isStreaming;
	CborIteratorToken token;
	const uint8_t *value;

	uint32_t objectSize;

	uint32_t stackSize;
	uint32_t stackCapacity;
	struct CborIteratorStackValue *currentStack;
	struct CborIteratorStackValue *stackHead;

	struct CborIteratorStackValue *extendedStack; // palloc'ed stack
	struct CborIteratorStackValue defaultStack[CBOR_STACK_DEFAULT_SIZE]; // preallocated stack
} CborIteratorContext;

typedef const struct CborData (*CborIteratorPathCallback) (void *);

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

/** returns pointer of current value beginning within data block
 * useful for value extraction
 */
const uint8_t *CborIteratorGetCurrentValuePtr(const CborIteratorContext *);

/* read until the end of current value, returns pointer to the next value
 * useful for value extraction
 *
 * data between ( CborIteratorGetCurrentValuePtr(iter), CborIteratorReadCurrentValue(iter) )
 * should be valid CBOR without header */
const uint8_t *CborIteratorReadCurrentValue(CborIteratorContext *);

/** Stop at i-th value in array. Iterator should be stopped at CborIteratorTokenBeginArray */
bool CborIteratorGetIth(CborIteratorContext *ctx, long int lindex);

/** Stop at value with specific object key. Iterator should be stopped at CborIteratorTokenBeginObject */
bool CborIteratorGetKey(CborIteratorContext *ctx, const char *, uint32_t);

/** Stop iterator at value, defined by path (e.g. { "objKey", "42", "valueKey" })
 *  Generic callback variant: callback mast return CborData { 0, NULL } to stop */
bool CborIteratorPath(CborIteratorContext *ctx, CborIteratorPathCallback, void *ptr);

/** Stop iterator at value, defined by path (e.g. "objKey", "42", "valueKey")
 *  null-terminated strings variant: it can be counted or null-terminated list
 *  if list is null-terminated, npath should be INT_MAX */
bool CborIteratorPathStrings(CborIteratorContext *ctx, const char **, int npath);

#endif /* INCLUDE_CBOR_ITER_H_ */
