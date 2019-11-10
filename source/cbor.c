
#include "cbor.h"

#include <limits.h>
#include <stdbool.h>

static void print_hex(const struct CborWriter *writer, const uint8_t *s, size_t size) {
	while (size > 0) {
		writer->format(writer->ctx, "%02x", *s++);
		-- size;
	}
}

static void CborValueToString(const struct CborWriter *writer, const CborIteratorContext *iter) {
	switch (CborIteratorGetType(iter)) {
	case CborTypeUnsigned: writer->format(writer->ctx, "%lu", CborIteratorGetUnsigned(iter)); break;
	case CborTypeNegative: writer->format(writer->ctx, "%ld", CborIteratorGetInteger(iter)); break;
	case CborTypeByteString:
		if (!iter->isStreaming) {
			writer->plain(writer->ctx, "\"hex(", 5);
		}
		print_hex(writer, CborIteratorGetBytePtr(iter), CborIteratorGetObjectSize(iter));
		if (!iter->isStreaming) {
			writer->plain(writer->ctx, ")\"", 2);
		}
		break;
	case CborTypeCharString:
		if (!iter->isStreaming) {
			writer->plain(writer->ctx, "\"", 1);
		}
		writer->plain(writer->ctx, CborIteratorGetCharPtr(iter), CborIteratorGetObjectSize(iter));
		if (!iter->isStreaming) {
			writer->plain(writer->ctx, "\"", 1);
		}
		break;
	case CborTypeArray:
	case CborTypeMap:
	case CborTypeTag:
		break;
	case CborTypeSimple:	writer->format(writer->ctx, "%lu", CborIteratorGetUnsigned(iter)); break;
	case CborTypeFloat:		writer->format(writer->ctx, "%.10e", CborIteratorGetFloat(iter)); break;
	case CborTypeFalse:		writer->plain(writer->ctx, "false", 5); break;
	case CborTypeTrue:		writer->plain(writer->ctx, "true", 4); break;
	case CborTypeNull:		writer->plain(writer->ctx, "null", 4); break;
	case CborTypeUndefined:	writer->plain(writer->ctx, "undefined", 9); break;
	default: break;
	}
}

static void CborValueIterToString(const struct CborWriter *writer, CborIteratorContext *iter) {
	switch (iter->token) {
	case CborIteratorTokenDone: break;
	case CborIteratorTokenKey:
		if (CborIteratorGetContainerPosition(iter) != 0) {
			writer->plain(writer->ctx, ";", 1);
		}
		CborValueToString(writer, iter);
		writer->plain(writer->ctx, ":", 1);
		break;
	case CborIteratorTokenValue:
		if (CborIteratorGetContainerType(iter) == (int)CborStackTypeArray && CborIteratorGetContainerPosition(iter) != 0) {
			writer->plain(writer->ctx, ",", 1);
		}
		CborValueToString(writer, iter);
		break;
	case CborIteratorTokenBeginArray:			writer->plain(writer->ctx, "[", 1); break;
	case CborIteratorTokenEndArray:				writer->plain(writer->ctx, "]", 1); break;
	case CborIteratorTokenBeginObject:			writer->plain(writer->ctx, "{", 1); break;
	case CborIteratorTokenEndObject:			writer->plain(writer->ctx, "}", 1); break;
	case CborIteratorTokenBeginByteStrings:		writer->plain(writer->ctx, "\"hex(", 5); break;
	case CborIteratorTokenEndByteStrings:		writer->plain(writer->ctx, ")\"", 2); break;
	case CborIteratorTokenBeginCharStrings:		writer->plain(writer->ctx, "\"", 1); break;
	case CborIteratorTokenEndCharStrings:		writer->plain(writer->ctx, "\"", 1); break;
	default: break;
	}
}

bool CborToString(const struct CborWriter *writer, const uint8_t *data, uint32_t size) {
	CborIteratorContext iter;
	if (CborIteratorInit(&iter, data, size)) {
		CborIteratorToken token = CborIteratorNext(&iter);
		while (token != CborIteratorTokenDone) {
			CborValueIterToString(writer, &iter);
			token = CborIteratorNext(&iter);
		}

		CborIteratorFinalize(&iter);
		return false;
	}
	return true;
}

bool CborIteratorValueToString(const struct CborWriter *writer, CborIteratorContext *iter) {
	switch (iter->token) {
	case CborIteratorTokenValue:
		CborValueToString(writer, iter);
		return true;
		break;
	case CborIteratorTokenBeginArray: {
		uint32_t stack = iter->stackSize;
		writer->plain(writer->ctx, "[", 1);
		while (CborIteratorNext(iter) != CborIteratorTokenEndArray && iter->stackSize > stack - 1) {
			CborValueIterToString(writer, iter);
		}
		CborIteratorNext(iter);
		writer->plain(writer->ctx, "]", 1);
		break;
	}
	case CborIteratorTokenBeginObject: {
		uint32_t stack = iter->stackSize;
		writer->plain(writer->ctx, "{", 1);
		while (CborIteratorNext(iter) != CborIteratorTokenEndObject && iter->stackSize > stack - 1) {
			CborValueIterToString(writer, iter);
		}
		CborIteratorNext(iter);
		writer->plain(writer->ctx, "}", 1);
		break;
	}
	case CborIteratorTokenBeginByteStrings:
		writer->plain(writer->ctx, "\"hex(", 5);
		while (CborIteratorNext(iter) != CborIteratorTokenEndByteStrings) {
			if (iter->token == CborIteratorTokenValue) {
				CborValueToString(writer, iter);
			}
		}
		writer->plain(writer->ctx, ")\"", 2);
		break;
	case CborIteratorTokenBeginCharStrings:
		writer->plain(writer->ctx, "\"", 1);
		while (CborIteratorNext(iter) != CborIteratorTokenEndCharStrings) {
			if (iter->token == CborIteratorTokenValue) {
				CborValueToString(writer, iter);
			}
		}
		writer->plain(writer->ctx, "\"", 1);
		break;
	default:
		return false;
		break;
	}
	return true;
}
