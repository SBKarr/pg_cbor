
#include "cbor_alloc.h"
#include "cbor_iter.h"
#include "cbor_typeinfo.h"

#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"

#include <string.h>
#include <limits.h>

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

static CborIteratorToken CborIteratorPushStack(CborIteratorContext *ctx, CborStackType type, uint32_t count, const uint8_t *ptr) {
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
	newStackValue->ptr = ptr;
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
		ctx->isStreaming = true;
		return CborIteratorTokenBeginByteStrings;
		break;
	case CborStackTypeCharString:
		ctx->isStreaming = true;
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
		ctx->isStreaming = false;
		return CborIteratorTokenEndByteStrings;
		break;
	case CborStackTypeCharString:
		ctx->isStreaming = false;
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
	const uint8_t *ptr;

	if (!CborDataOffset(&ctx->current, ctx->objectSize)) {
		if (ctx->stackHead) {
			ctx->token = CborIteratorPopStack(ctx);
		} else {
			ctx->token = CborIteratorTokenDone;
		}
		ctx->value = ctx->current.ptr;
		return ctx->token;
	}

	head = ctx->stackHead;

	// pop stack value if all objects was parsed
	if (head && head->position >= head->count) {
		ctx->token = CborIteratorPopStack(ctx);
		ctx->value = ctx->current.ptr;
		return ctx->token;
	}

	// read flag value
	ptr = ctx->current.ptr;
	type = CborDataGetUnsigned(&ctx->current);
	CborDataOffset(&ctx->current, 1);

	// pop stack value for undefined length container
	if (head && head->count == UINT32_MAX && type == (CborFlagsUndefinedLength | CborMajorTypeEncodedSimple)) {
		ctx->token = CborIteratorPopStack(ctx);
		return ctx->token;
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

	ctx->value = ptr;
	if (nextStackType != CborStackTypeNone) {
		if (ctx->info == CborFlagsUndefinedLength) {
			ctx->token = CborIteratorPushStack(ctx, nextStackType, UINT32_MAX, ptr);
		} else {
			ctx->token = CborIteratorPushStack(ctx, nextStackType, CborDataReadUnsignedValue(&ctx->current, ctx->info), ptr);
		}
		return ctx->token;
	}

	ctx->token = (head && head->type == CborStackTypeObject)
		? ( head->position % 2 == 1 ? CborIteratorTokenKey : CborIteratorTokenValue )
		: CborIteratorTokenValue;
	return ctx->token;
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
		if (ctx->stackHead->count == UINT32_MAX) {
			return ctx->stackHead->count;
		}
		return (ctx->stackHead->type == CborStackTypeObject) ? ctx->stackHead->count / 2 : ctx->stackHead->count;
	}
}

uint32_t CborIteratorGetContainerPosition(const CborIteratorContext *ctx) {
	if (!ctx->stackHead) {
		return 0;
	} else {
		return (ctx->stackHead->type == CborStackTypeObject) ? (ctx->stackHead->position - 1) / 2 : (ctx->stackHead->position - 1);
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

const uint8_t *CborIteratorGetCurrentValuePtr(const CborIteratorContext *ctx) {
	switch (ctx->token) {
	case CborIteratorTokenKey:
	case CborIteratorTokenValue:
		return ctx->value;
		break;
	case CborIteratorTokenBeginArray:
	case CborIteratorTokenBeginObject:
	case CborIteratorTokenBeginByteStrings:
	case CborIteratorTokenBeginCharStrings:
		return ctx->stackHead->ptr;
		break;
	default: break;
	}
	return NULL;
}

const uint8_t *CborIteratorReadCurrentValue(CborIteratorContext *iter) {
	switch (iter->token) {
	case CborIteratorTokenValue:
	case CborIteratorTokenKey:
		CborIteratorNext(iter);
		return iter->value;
		break;
	case CborIteratorTokenBeginArray: {
		uint32_t stack = iter->stackSize;
		while (CborIteratorNext(iter) != CborIteratorTokenEndArray && iter->stackSize > stack - 1) { }
		CborIteratorNext(iter);
		return iter->value;
		break;
	}
	case CborIteratorTokenBeginObject: {
		uint32_t stack = iter->stackSize;
		while (CborIteratorNext(iter) != CborIteratorTokenEndObject && iter->stackSize > stack - 1) { }
		CborIteratorNext(iter);
		return iter->value;
		break;
	}
	case CborIteratorTokenBeginByteStrings:
		while (CborIteratorNext(iter) != CborIteratorTokenEndByteStrings) { }
		CborIteratorNext(iter);
		return iter->value;
		break;
	case CborIteratorTokenBeginCharStrings:
		while (CborIteratorNext(iter) != CborIteratorTokenEndCharStrings) { }
		CborIteratorNext(iter);
		return iter->value;
		break;
	default:
		return NULL;
		break;
	}
	return NULL;
}

bool CborIteratorGetIth(CborIteratorContext *ctx, long int lindex) {
	uint32_t stackSize;
	struct CborIteratorStackValue *h;

	if (!ctx->stackHead || ctx->stackHead->type != CborStackTypeArray || ctx->token != CborIteratorTokenBeginArray) {
		return false;
	}

	if (lindex >= 0) {
		if ((uint32_t)lindex >= ctx->stackHead->count) {
			return false;
		}
	} else if (lindex < 0) {
		if (ctx->stackHead->count != UINT32_MAX) {
			if ((uint32_t)-lindex > ctx->stackHead->count) {
				return false;
			} else {
				lindex = ctx->stackHead->count + lindex;
			}
		} else {
			return false;
		}
	}

	stackSize = ctx->stackSize;
	h = ctx->stackHead;

	while ((h->position != lindex || ctx->stackSize > stackSize) && ctx->token != CborIteratorTokenDone) {
		CborIteratorNext(ctx);
	}

	if (ctx->stackSize < stackSize || ctx->token == CborIteratorTokenDone) {
		return false;
	} else {
		CborIteratorNext(ctx);
		return true;
	}
}

bool CborIteratorGetKey(CborIteratorContext *ctx, const char *str, uint32_t size) {
	uint32_t stackSize;
	const char *tmpstr = str;
	uint32_t tmpsize = size;

	if (!ctx->stackHead || ctx->stackHead->type != CborStackTypeObject || ctx->token != CborIteratorTokenBeginObject) {
		return false;
	}

	stackSize = ctx->stackSize;
	while (ctx->token != CborIteratorTokenDone && ctx->stackSize >= stackSize) {
		CborIteratorToken token = CborIteratorNext(ctx);
		if (ctx->stackSize == stackSize) {
			switch (token) {
			case CborIteratorTokenKey:
				if (ctx->isStreaming) {
					if (tmpstr && ctx->objectSize <= size) {
						if (memcmp(str, ctx->current.ptr, ctx->objectSize) == 0) {
							tmpstr += ctx->objectSize;
							size -= ctx->objectSize;
						} else {
							// invalid key
							tmpstr = NULL;
						}
					} else {
						tmpstr = NULL;
					}
				} else if (ctx->type == CborMajorTypeByteString || ctx->type == CborMajorTypeCharString) {
					if (ctx->objectSize == size && memcmp(str, ctx->current.ptr, size) == 0) {
						CborIteratorNext(ctx);
						return true;
					}
				}
				break;
			case CborIteratorTokenValue:
				if (tmpsize == 0) {
					return true;
				}
				tmpstr = str;
				tmpsize = size;
				break;
			default:
				break;
			}
		}
	}

	if (ctx->stackSize < stackSize || ctx->token == CborIteratorTokenDone) {
		return false;
	} else {
		return true;
	}
}

bool CborIteratorPath(CborIteratorContext *ctx, CborIteratorPathCallback cb, void *ptr) {
	struct CborData data = cb(ptr);

	CborIteratorToken token = CborIteratorNext(ctx);
	switch (token) {
	case CborIteratorTokenBeginArray:
	case CborIteratorTokenBeginObject:
		if (data.ptr == NULL) {
			return true;
		}
		// continue
		break;
	case CborIteratorTokenValue:
	case CborIteratorTokenBeginByteStrings:
	case CborIteratorTokenBeginCharStrings:
		return data.ptr == NULL;
		break;
	default:
		return false;
		break;
	}

	while (data.ptr) {
		switch (token) {
		case CborIteratorTokenBeginArray: {
			const char *indextext = (const char *)data.ptr;
			char *endptr;
			long int lindex = strtol(indextext, &endptr, 10);
			if (endptr == indextext || *endptr != '\0' || lindex > INT_MAX || lindex < INT_MIN) {
				return false;
			}

			if (!CborIteratorGetIth(ctx, lindex)) {
				return false;
			}
			token = ctx->token;
			break;
		}
		case CborIteratorTokenBeginObject:
			if (!CborIteratorGetKey(ctx, (const char *)data.ptr, data.size)) {
				return false;
			}
			token = ctx->token;
			break;
		default:
			return false;
			break;
		}
		data = cb(ptr);
	}

	return true;
}

struct CborIteratorPathStringsData {
	const char **path;
	int npath;
};

static CborData CborIteratorPathStringsIter(struct CborIteratorPathStringsData *data) {
	if (data->npath > 0) {
		CborData ret = {(*data->path) ? strlen(*data->path) : 0, (const uint8_t *)*data->path};
		-- data->npath;
		++ data->path;
		return ret;
	} else {
		CborData ret = { 0, NULL };
		return ret;
	}
}

bool CborIteratorPathStrings(CborIteratorContext *ctx, const char **path, int npath) {
	struct CborIteratorPathStringsData data = {
		path,
		npath
	};

	return CborIteratorPath(ctx, (CborIteratorPathCallback)&CborIteratorPathStringsIter, &data);
}
