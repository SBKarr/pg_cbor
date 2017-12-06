
#include "cbor_alloc.h"
#include "cbor_iter.h"
#include "cbor_typeinfo.h"

#include <string.h>

static inline uint32_t get_cbor_integer_length(uint8_t info) {
	switch (info) {
	case CborFlagsAdditionalNumber8Bit: return 1; break;
	case CborFlagsAdditionalNumber16Bit: return 2; break;
	case CborFlagsAdditionalNumber32Bit: return 4; break;
	case CborFlagsAdditionalNumber64Bit: return 8; break;
	default: return 0; break;
	}

	return 0;
}

bool CborIteratorInit(CborIteratorContext *ctx, const uint8_t *data, size_t size) {
	memset(ctx, 0, sizeof(CborIteratorContext));

	if (data_is_cbor(data, size)) { // prefixed block
		data += 3;
		size -= 3;
	} else if (size == 0) { // non-prefixed
		return false;
	}

	ctx->current.ptr = data;
	ctx->current.size = size;

	ctx->currentStack = ctx->defaultStack;
	ctx->stackCapacity = CBOR_STACK_DEFAULT_SIZE;

	return true;
}

void CborIteratorFinalize(CborIteratorContext *ctx) {
	if (ctx->extendedStack) {
		CborFree(ctx->extendedStack);
		ctx->extendedStack = NULL;
	}
}

void CborIteratorReset(CborIteratorContext *ctx) {
	if (ctx->extendedStack) {
		CborFree(ctx->extendedStack);
	}
	memset(ctx, 0, sizeof(CborIteratorContext));
}

static CborIteratorToken CborIteratorPushStack(CborIteratorContext *ctx, CborStackType type, uint32_t count) {
	struct CborIteratorStackValue * newStackValue;

	if (ctx->stackCapacity == ctx->stackSize) {
		if (!ctx->extendedStack) {
			ctx->extendedStack = CborAlloc(sizeof(struct CborIteratorStackValue) * ctx->stackCapacity * 2);
			memcpy(ctx->extendedStack, ctx->defaultStack, sizeof(struct CborIteratorStackValue) * ctx->stackCapacity);
			ctx->currentStack = ctx->extendedStack;
		} else {
			ctx->currentStack = ctx->extendedStack = CborRealloc(ctx->extendedStack, sizeof(struct CborIteratorStackValue) * ctx->stackCapacity * 2);
			ctx->stackCapacity *= 2;
		}
		ctx->stackCapacity *= 2;
	}

	newStackValue = &ctx->currentStack[ctx->stackSize];
	newStackValue->type = type;
	newStackValue->position = 0;
	if (type == CborStackTypeObject && count != UINT32_MAX) {
		newStackValue->count = count * 2;
	} else {
		newStackValue->count = count;
	}

	ctx->objectSize = 0;
	ctx->stackHead = newStackValue;
	++ ctx->stackSize;

	switch (type) {
	case CborStackTypeArray:
		return CborIteratorTokenBeginArray;
		break;
	case CborStackTypeObject:
		return CborIteratorTokenBeginObject;
		break;
	case CborStackTypeByteString:
		return CborIteratorTokenBeginByteStrings;
		break;
	case CborStackTypeCharString:
		return CborIteratorTokenBeginCharStrings;
		break;
	default: break;
	}
	return CborIteratorTokenDone;
}

static CborIteratorToken CborIteratorPopStack(CborIteratorContext *ctx) {
	CborStackType type;

	if (!ctx->stackHead) {
		return CborIteratorTokenDone;
	}

	type = ctx->stackHead->type;
	-- ctx->stackSize;

	ctx->objectSize = 0;
	if (ctx->stackSize > 0) {
		ctx->stackHead = &ctx->currentStack[ctx->stackSize - 1];
	} else {
		ctx->stackHead = NULL;
	}

	switch (type) {
	case CborStackTypeArray:
		return CborIteratorTokenEndArray;
		break;
	case CborStackTypeObject:
		return CborIteratorTokenEndObject;
		break;
	case CborStackTypeByteString:
		return CborIteratorTokenEndByteStrings;
		break;
	case CborStackTypeCharString:
		return CborIteratorTokenEndCharStrings;
		break;
	default: break;
	}
	return CborIteratorTokenDone;
}

CborIteratorToken CborIteratorNext(CborIteratorContext *ctx) {
	struct CborIteratorStackValue *head;
	CborStackType nextStackType;
	uint8_t type;

	if (!CborDataOffset(&ctx->current, ctx->objectSize)) {
		if (ctx->stackHead) {
			return CborIteratorPopStack(ctx);
		}
		return CborIteratorTokenDone;
	}

	head = ctx->stackHead;

	// pop stack value if all objects was parsed
	if (head && head->position >= head->count) {
		return CborIteratorPopStack(ctx);
	}

	// read flag value
	type = CborDataGetUnsigned(&ctx->current);
	CborDataOffset(&ctx->current, 1);

	// pop stack value for undefined length container
	if (head && head->count == UINT32_MAX && type == (CborFlagsUndefinedLength | CborMajorTypeEncodedSimple)) {
		return CborIteratorPopStack(ctx);
	}

	ctx->type = (type & CborFlagsMajorTypeMaskEncoded) >> CborFlagsMajorTypeShift;
	ctx->info = type & CborFlagsAdditionalInfoMask;

	nextStackType = CborStackTypeNone;

	switch (ctx->type) {
	case CborMajorTypeUnsigned:
	case CborMajorTypeNegative:
		ctx->objectSize = get_cbor_integer_length(ctx->info);
		if (head) { ++ head->position; }

		break;
	case CborMajorTypeTag:
		ctx->objectSize = get_cbor_integer_length(ctx->info);
		break;

	case CborMajorTypeSimple:
		ctx->objectSize = get_cbor_integer_length(ctx->info);
		if (head) { ++ head->position; }

		break;
	case CborMajorTypeByteString:
		if (ctx->info == CborFlagsUndefinedLength) {
			nextStackType = CborStackTypeByteString;
		} else {
			ctx->objectSize = CborDataReadUnsignedValue(&ctx->current, ctx->info);
		}
		if (head) { ++ head->position; }
		break;
	case CborMajorTypeCharString:
		if (ctx->info == CborFlagsUndefinedLength) {
			nextStackType = CborStackTypeCharString;
		} else {
			ctx->objectSize = CborDataReadUnsignedValue(&ctx->current, ctx->info);
		}
		if (head) { ++ head->position; }
		break;
	case CborMajorTypeArray:
		nextStackType = CborStackTypeArray;
		if (head) { ++ head->position; }
		break;
	case CborMajorTypeMap:
		nextStackType = CborStackTypeObject;
		if (head) { ++ head->position; }
		break;
	default: break;
	}

	if (nextStackType != CborStackTypeNone) {
		if (ctx->info == CborFlagsUndefinedLength) {
			return CborIteratorPushStack(ctx, nextStackType, UINT32_MAX);
		} else {
			return CborIteratorPushStack(ctx, nextStackType, CborDataReadUnsignedValue(&ctx->current, ctx->info));
		}
	}

	return (head && head->type == CborStackTypeObject)
		? ( head->position % 2 == 1 ? CborIteratorTokenKey : CborIteratorTokenValue )
		: CborIteratorTokenValue;
}

CborType CborIteratorGetType(const CborIteratorContext *ctx) {
	switch (ctx->type) {
	case CborMajorTypeUnsigned:
		return CborTypeUnsigned;
		break;
	case CborMajorTypeNegative:
		return CborTypeNegative;
		break;
	case CborMajorTypeByteString:
		return CborTypeByteString;
		break;
	case CborMajorTypeCharString:
		return CborTypeCharString;
		break;
	case CborMajorTypeArray:
		return CborTypeArray;
		break;
	case CborMajorTypeMap:
		return CborTypeMap;
		break;
	case CborMajorTypeTag:
		return CborTypeTag;
		break;
	case CborMajorTypeSimple:
		switch (ctx->info) {
		case CborSimpleValueFalse: return CborTypeFalse; break;
		case CborSimpleValueTrue: return CborTypeTrue; break;
		case CborSimpleValueNull: return CborTypeNull; break;
		case CborSimpleValueUndefined: return CborTypeUndefined; break;
		case CborFlagsAdditionalFloat16Bit:
		case CborFlagsAdditionalFloat32Bit:
		case CborFlagsAdditionalFloat64Bit:
			return CborTypeFloat;
			break;
		}
		return CborTypeSimple;
		break;
	default: break;
	}

	return CborTypeUnknown;
}

CborStackType CborIteratorGetContainerType(const CborIteratorContext *ctx) {
	if (!ctx->stackHead) {
		return CborStackTypeNone;
	} else {
		return ctx->stackHead->type;
	}
}

uint32_t CborIteratorGetContainerSize(const CborIteratorContext *ctx) {
	if (!ctx->stackHead) {
		return 0;
	} else {
		return (ctx->stackHead->type == CborStackTypeObject) ? ctx->stackHead->count / 2 : ctx->stackHead->count;
	}
}

uint32_t CborIteratorGetContainerPosition(const CborIteratorContext *ctx) {
	if (!ctx->stackHead) {
		return 0;
	} else {
		return (ctx->stackHead->type == CborStackTypeObject) ? ctx->stackHead->position / 2 : ctx->stackHead->position;
	}
}

uint32_t CborIteratorGetObjectSize(const CborIteratorContext *ctx) {
	return ctx->objectSize;
}

int64_t CborIteratorGetInteger(const CborIteratorContext *ctx) {
	int64_t ret = 0;
	switch (CborIteratorGetType(ctx)) {
	case CborTypeUnsigned:
	case CborTypeTag:
	case CborTypeSimple:
		ret = CborDataGetUnsignedValue(&ctx->current, ctx->info);
		break;
	case CborTypeNegative:
		ret = -1 - CborDataGetUnsignedValue(&ctx->current, ctx->info);
		break;
	default: break;
	}
	return ret;
}

uint64_t CborIteratorGetUnsigned(const CborIteratorContext *ctx) {
	uint64_t ret = 0;
	switch (CborIteratorGetType(ctx)) {
	case CborTypeUnsigned:
	case CborTypeTag:
	case CborTypeSimple:
		ret = CborDataGetUnsignedValue(&ctx->current, ctx->info);
		break;
	default: break;
	}
	return ret;
}

double CborIteratorGetFloat(const CborIteratorContext *ctx) {
	double ret = 0.0;
	switch (CborIteratorGetType(ctx)) {
	case CborTypeFloat:
		switch (ctx->info) {
		case CborFlagsAdditionalFloat16Bit:
			ret = CborDataGetFloat16(&ctx->current);
			break;
		case CborFlagsAdditionalFloat32Bit:
			ret = CborDataGetFloat32(&ctx->current);
			break;
		case CborFlagsAdditionalFloat64Bit:
			ret = CborDataGetFloat64(&ctx->current);
			break;
		default: break;
		}
		break;
	default: break;
	}
	return ret;
}

const char *CborIteratorGetCharPtr(const CborIteratorContext *ctx) {
	const char *ret = NULL;
	switch (CborIteratorGetType(ctx)) {
	case CborTypeCharString:
		ret = (const char *)ctx->current.ptr;
		break;
	default: break;
	}
	return ret;
}

const uint8_t *CborIteratorGetBytePtr(const CborIteratorContext *ctx) {
	const uint8_t *ret = NULL;
	switch (CborIteratorGetType(ctx)) {
	case CborTypeByteString:
		ret = ctx->current.ptr;
		break;
	default: break;
	}
	return ret;
}
