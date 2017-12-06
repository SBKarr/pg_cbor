
#include "pg_cbor.h"

static void print_hex(StringInfo str, const uint8_t *s, size_t size) {
	while (size > 0) {
		appendStringInfo(str, "%02x", *s++);
		-- size;
	}
}

static void CborValueToString(StringInfo str, const CborIteratorContext *iter, bool stream) {
	switch (CborIteratorGetType(iter)) {
	case CborTypeUnsigned: appendStringInfo(str, "%lu", CborIteratorGetUnsigned(iter)); break;
	case CborTypeNegative: appendStringInfo(str, "%ld", CborIteratorGetInteger(iter)); break;
	case CborTypeByteString:
		if (!stream) {
			appendStringInfoString(str, "\"hex(");
		}
		print_hex(str, CborIteratorGetBytePtr(iter), CborIteratorGetObjectSize(iter));
		if (!stream) {
			appendStringInfoString(str, ")\"");
		}
		break;
	case CborTypeCharString:
		if (!stream) {
			appendStringInfoString(str, "\"");
		}
		appendBinaryStringInfo(str, CborIteratorGetCharPtr(iter), CborIteratorGetObjectSize(iter));
		if (!stream) {
			appendStringInfoString(str, "\"");
		}
		break;
	case CborTypeArray:
	case CborTypeMap:
	case CborTypeTag:
		break;
	case CborTypeSimple: appendStringInfo(str, "%lu", CborIteratorGetUnsigned(iter)); break;
	case CborTypeFloat: appendStringInfo(str, "%.10e", CborIteratorGetFloat(iter)); break;
	case CborTypeFalse: appendStringInfoString(str, "false"); break;
	case CborTypeTrue: appendStringInfoString(str, "true"); break;
	case CborTypeNull: appendStringInfoString(str, "null"); break;
	case CborTypeUndefined: appendStringInfoString(str, "undefined"); break;
	default: break;
	}
}

void CborToString(StringInfo str, const uint8_t *data, uint32_t size) {
	bool stream = false;
	CborIteratorContext iter;
	if (CborIteratorInit(&iter, data, size)) {
		CborIteratorToken token = CborIteratorNext(&iter);
		while (token != CborIteratorTokenDone) {
			switch (token) {
			case CborIteratorTokenDone:
				break;
			case CborIteratorTokenKey:
				if (CborIteratorGetContainerPosition(&iter) != 0) {
					appendStringInfoString(str, ",");
				}
				CborValueToString(str, &iter, stream);
				appendStringInfoString(str, ":");
				break;
			case CborIteratorTokenValue:
				if (CborIteratorGetContainerType(&iter) == (int)CborTypeArray && CborIteratorGetContainerPosition(&iter) != 0) {
					appendStringInfoString(str, ",");
				}
				CborValueToString(str, &iter, stream);
				break;
			case CborIteratorTokenBeginArray:
				appendStringInfoChar(str, '[');
				break;
			case CborIteratorTokenEndArray:
				appendStringInfoChar(str, ']');
				break;
			case CborIteratorTokenBeginObject:
				appendStringInfoChar(str, '{');
				break;
			case CborIteratorTokenEndObject:
				appendStringInfoChar(str, '}');
				break;
			case CborIteratorTokenBeginByteStrings:
				appendStringInfoString(str, "\"hex(");
				stream = true;
				break;
			case CborIteratorTokenEndByteStrings:
				appendStringInfoString(str, ")\"");
				stream = false;
				break;
			case CborIteratorTokenBeginCharStrings:
				appendStringInfoChar(str, '\"');
				stream = true;
				break;
			case CborIteratorTokenEndCharStrings:
				appendStringInfoChar(str, '\"');
				stream = false;
				break;
			default:
				break;
			}
			token = CborIteratorNext(&iter);
		}

		CborIteratorFinalize(&iter);
	}
}
