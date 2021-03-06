
#include "pg_cbor.h"
#include "utils/builtins.h"
#include "utils/array.h"
#include "catalog/pg_type.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(is_cbor);
PG_FUNCTION_INFO_V1(cbor_to_string);
PG_FUNCTION_INFO_V1(cbor_extract_path);
PG_FUNCTION_INFO_V1(cbor_extract_path_text);

PG_FUNCTION_INFO_V1(cbor_path_as_text);
PG_FUNCTION_INFO_V1(cbor_path_as_bytes);
PG_FUNCTION_INFO_V1(cbor_path_as_int);
PG_FUNCTION_INFO_V1(cbor_path_as_float);
PG_FUNCTION_INFO_V1(cbor_path_as_bool);

Datum
is_cbor(PG_FUNCTION_ARGS) {
	bytea *ptr;
	size_t bsize;
	const uint8_t *data;

	if (PG_ARGISNULL(0)) {
	    PG_RETURN_BOOL(0);
	}

	ptr = PG_GETARG_BYTEA_P(0);
	bsize = VARSIZE(ptr) - VARHDRSZ;
	data = (const uint8_t *)VARDATA(ptr);

	if (data_is_cbor(data, bsize)) {
		PG_RETURN_BOOL(1);
	}

    PG_RETURN_BOOL(0);
}

Datum
cbor_to_string(PG_FUNCTION_ARGS) {
	bytea *ptr;
	size_t bsize;
	const uint8_t *data;
	StringInfoData str;
	struct CborWriter writer;
	text *ret;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	ptr = PG_GETARG_BYTEA_P(0);
	bsize = VARSIZE(ptr) - VARHDRSZ;
	data = (const uint8_t *)VARDATA(ptr);
	if (data_is_cbor(data, bsize)) {
		initStringInfo(&str);
		writer.plain = (CborWriterPlain)appendBinaryStringInfo;
		writer.format = (CborWriterFormat)appendStringInfo;
		writer.ctx = &str;

		CborToString(&writer, data, bsize);
		ret = cstring_to_text_with_len(str.data, str.len);
		pfree(str.data);
		PG_RETURN_TEXT_P(ret);
	} else {
		PG_RETURN_NULL();
	}
}

struct PgCborIteratorPathData {
	Datum *path;
	int npath;
};

static CborData PgCborIteratorPathIter(struct PgCborIteratorPathData *data) {
	if (data->npath > 0) {
		CborData ret;
		ret.size = VARSIZE(*data->path) - VARHDRSZ;
		ret.ptr = (const uint8_t *)VARDATA(*data->path);

		-- data->npath;
		++ data->path;
		return ret;
	} else {
		CborData ret;
		ret.size = 0;
		ret.ptr = NULL;
		return ret;
	}
}

static bool PgCborIteratorPath(CborIteratorContext *ctx, Datum *path, int npath) {
	struct PgCborIteratorPathData data;
	data.path = path;
	data.npath = npath;

	return CborIteratorPath(ctx, (CborIteratorPathCallback)&PgCborIteratorPathIter, &data);
}

Datum
cbor_extract_path(PG_FUNCTION_ARGS) {
	bytea *ptr;
	ArrayType *path;
	size_t bsize;
	const uint8_t *data;

	Datum *pathtext;
	bool *pathnulls;
	int	npath;

	CborIteratorContext iter;
	const uint8_t *begin;
	const uint8_t *end;

	int bc;
	bytea *result;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	ptr = PG_GETARG_BYTEA_P(0);
	path = PG_GETARG_ARRAYTYPE_P(1);
	bsize = VARSIZE(ptr) - VARHDRSZ;
	data = (const uint8_t *)VARDATA(ptr);

	if (array_contains_nulls(path)) {
		elog(ERROR, "Invalid path data");
		PG_RETURN_NULL();
	}

	if (!data_is_cbor(data, bsize)) {
		PG_RETURN_NULL();
	}

	deconstruct_array(path, TEXTOID, -1, false, 'i', &pathtext, &pathnulls, &npath);

	if (npath <= 0) {
		PG_RETURN_BYTEA_P(ptr);
	}

	if (CborIteratorInit(&iter, data, bsize)) {
		if (PgCborIteratorPath(&iter, pathtext, npath)) {
			begin = CborIteratorGetCurrentValuePtr(&iter);
			end = CborIteratorReadCurrentValue(&iter);

			bc = (end - begin) + CborHeaderSize;
			result = palloc(bc + VARHDRSZ);

			memcpy(VARDATA(result), CborHeaderData, CborHeaderSize);
			memcpy(VARDATA(result) + CborHeaderSize, begin, end - begin);
			SET_VARSIZE(result, bc + VARHDRSZ);

			CborIteratorFinalize(&iter);
			PG_RETURN_BYTEA_P(result);
		}
	}
	PG_RETURN_NULL();
}

static text *
pg_cbor_to_text(CborIteratorContext *iter) {
	text *ret = NULL;
	if (CborIteratorGetType(iter) == CborTypeCharString) {
		if (iter->token == CborIteratorTokenBeginCharStrings) {
			StringInfoData str;
			initStringInfo(&str);
			while (CborIteratorNext(iter) != CborIteratorTokenEndCharStrings) {
				if (iter->token == CborIteratorTokenValue) {
					appendBinaryStringInfo(&str, CborIteratorGetCharPtr(iter), CborIteratorGetObjectSize(iter));
				}
			}
			ret = cstring_to_text_with_len(str.data, str.len);
			pfree(str.data);
		} else {
			ret = cstring_to_text_with_len(CborIteratorGetCharPtr(iter), CborIteratorGetObjectSize(iter));
		}
	}
	return ret;
}

static bytea *
pg_cbor_to_bytes(CborIteratorContext *iter) {
	bytea *result = NULL;
	const uint8_t *ptr;
	uint32_t len;
	if (CborIteratorGetType(iter) == CborTypeByteString) {
		if (iter->token == CborIteratorTokenBeginByteStrings) {
			StringInfoData str;
			initStringInfo(&str);
			while (CborIteratorNext(iter) != CborIteratorTokenEndByteStrings) {
				if (iter->token == CborIteratorTokenValue) {
					appendBinaryStringInfo(&str, CborIteratorGetCharPtr(iter), CborIteratorGetObjectSize(iter));
				}
			}

			result = palloc(str.len + VARHDRSZ);
			memcpy(VARDATA(result), str.data, str.len);
			SET_VARSIZE(result, str.len + VARHDRSZ);

			pfree(str.data);
		} else {
			ptr = CborIteratorGetBytePtr(iter);
			len = CborIteratorGetObjectSize(iter);

			result = palloc(len + VARHDRSZ);
			memcpy(VARDATA(result), ptr, len);
			SET_VARSIZE(result, len + VARHDRSZ);
		}
	}
	return result;
}

Datum
cbor_extract_path_text(PG_FUNCTION_ARGS) {
	bytea *ptr;
	ArrayType *path;

	size_t bsize;
	const uint8_t *data;

	Datum *pathtext;
	bool *pathnulls;
	int	npath;

	struct CborWriter writer;
	StringInfoData str;
	CborIteratorContext iter;
	text *ret = NULL;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	ptr = PG_GETARG_BYTEA_P(0);
	path = PG_GETARG_ARRAYTYPE_P(1);

	bsize = VARSIZE(ptr) - VARHDRSZ;
	data = (const uint8_t *)VARDATA(ptr);

	if (array_contains_nulls(path)) {
		elog(ERROR, "Invalid path data");
		PG_RETURN_NULL();
	}

	if (!data_is_cbor(data, bsize)) {
		PG_RETURN_NULL();
	}

	deconstruct_array(path, TEXTOID, -1, false, 'i', &pathtext, &pathnulls, &npath);

	if (npath <= 0) {
		initStringInfo(&str);
		writer.plain = (CborWriterPlain)appendBinaryStringInfo;
		writer.format = (CborWriterFormat)appendStringInfo;
		writer.ctx = &str;

		CborToString(&writer, data, bsize);

		ret = cstring_to_text_with_len(str.data, str.len);
		pfree(str.data);
		PG_RETURN_TEXT_P(ret);
	}

	if (CborIteratorInit(&iter, data, bsize)) {
		if (PgCborIteratorPath(&iter, pathtext, npath)) {
			if (CborIteratorGetType(&iter) == CborTypeCharString) {
				ret = pg_cbor_to_text(&iter);
			} else {
				initStringInfo(&str);
				writer.plain = (CborWriterPlain)appendBinaryStringInfo;
				writer.format = (CborWriterFormat)appendStringInfo;
				writer.ctx = &str;
				CborIteratorValueToString(&writer, &iter);
				ret = cstring_to_text_with_len(str.data, str.len);
				pfree(str.data);
			}

			CborIteratorFinalize(&iter);
			if (ret) {
				PG_RETURN_TEXT_P(ret);
			}
		}
	}
	PG_RETURN_NULL();
}

Datum
cbor_path_as_text(PG_FUNCTION_ARGS) {
	bytea *ptr;
	ArrayType *path;

	size_t bsize;
	const uint8_t *data;

	Datum *pathtext;
	bool *pathnulls;
	int	npath;

	CborIteratorContext iter;
	text *ret = NULL;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	ptr = PG_GETARG_BYTEA_P(0);
	path = PG_GETARG_ARRAYTYPE_P(1);

	bsize = VARSIZE(ptr) - VARHDRSZ;
	data = (const uint8_t *)VARDATA(ptr);

	if (array_contains_nulls(path)) {
		elog(ERROR, "Invalid path data");
		PG_RETURN_NULL();
	}

	if (!data_is_cbor(data, bsize)) {
		PG_RETURN_NULL();
	}

	deconstruct_array(path, TEXTOID, -1, false, 'i', &pathtext, &pathnulls, &npath);

	if (CborIteratorInit(&iter, data, bsize)) {
		if (npath <= 0) {
			CborIteratorNext(&iter);
			ret = pg_cbor_to_text(&iter);
			CborIteratorFinalize(&iter);
			if (ret) {
				PG_RETURN_TEXT_P(ret);
			}
		} else if (PgCborIteratorPath(&iter, pathtext, npath)) {
			ret = pg_cbor_to_text(&iter);
			CborIteratorFinalize(&iter);
			if (ret) {
				PG_RETURN_TEXT_P(ret);
			}
		}
	}
	PG_RETURN_NULL();
}

Datum
cbor_path_as_bytes(PG_FUNCTION_ARGS) {
	bytea *ptr;
	ArrayType *path;

	size_t bsize;
	const uint8_t *data;

	Datum *pathtext;
	bool *pathnulls;
	int	npath;

	CborIteratorContext iter;
	bytea *ret = NULL;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	ptr = PG_GETARG_BYTEA_P(0);
	path = PG_GETARG_ARRAYTYPE_P(1);

	bsize = VARSIZE(ptr) - VARHDRSZ;
	data = (const uint8_t *)VARDATA(ptr);

	if (array_contains_nulls(path)) {
		elog(ERROR, "Invalid path data");
		PG_RETURN_NULL();
	}

	if (!data_is_cbor(data, bsize)) {
		PG_RETURN_NULL();
	}

	deconstruct_array(path, TEXTOID, -1, false, 'i', &pathtext, &pathnulls, &npath);

	if (CborIteratorInit(&iter, data, bsize)) {
		if (npath <= 0) {
			CborIteratorNext(&iter);
			ret = pg_cbor_to_bytes(&iter);
			CborIteratorFinalize(&iter);
			if (ret) {
				PG_RETURN_BYTEA_P(ptr);
			}
		} else if (PgCborIteratorPath(&iter, pathtext, npath)) {
			ret = pg_cbor_to_bytes(&iter);
			CborIteratorFinalize(&iter);
			if (ret) {
				PG_RETURN_BYTEA_P(ptr);
			}
		}
	}
	PG_RETURN_NULL();
}

Datum
cbor_path_as_int(PG_FUNCTION_ARGS) {
	bytea *ptr;
	ArrayType *path;

	size_t bsize;
	const uint8_t *data;

	Datum *pathtext;
	bool *pathnulls;
	int	npath;

	CborIteratorContext iter;
	int64_t ret;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	ptr = PG_GETARG_BYTEA_P(0);
	path = PG_GETARG_ARRAYTYPE_P(1);

	bsize = VARSIZE(ptr) - VARHDRSZ;
	data = (const uint8_t *)VARDATA(ptr);

	if (array_contains_nulls(path)) {
		elog(ERROR, "Invalid path data");
		PG_RETURN_NULL();
	}

	if (!data_is_cbor(data, bsize)) {
		PG_RETURN_NULL();
	}

	deconstruct_array(path, TEXTOID, -1, false, 'i', &pathtext, &pathnulls, &npath);

	if (CborIteratorInit(&iter, data, bsize)) {
		if (npath <= 0) {
			CborIteratorNext(&iter);
			if (CborIteratorGetType(&iter) == CborTypeUnsigned || CborIteratorGetType(&iter) == CborTypeNegative) {
				ret = CborIteratorGetInteger(&iter);
				CborIteratorFinalize(&iter);
				PG_RETURN_INT64(ret);
			}
			CborIteratorFinalize(&iter);
		} else if (PgCborIteratorPath(&iter, pathtext, npath)) {
			if (CborIteratorGetType(&iter) == CborTypeUnsigned || CborIteratorGetType(&iter) == CborTypeNegative) {
				ret = CborIteratorGetInteger(&iter);
				CborIteratorFinalize(&iter);
				PG_RETURN_INT64(ret);
			}
			CborIteratorFinalize(&iter);
		}
	}
	PG_RETURN_NULL();
}

Datum
cbor_path_as_float(PG_FUNCTION_ARGS) {
	bytea *ptr;
	ArrayType *path;

	size_t bsize;
	const uint8_t *data;

	Datum *pathtext;
	bool *pathnulls;
	int	npath;

	CborIteratorContext iter;
	double ret;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	ptr = PG_GETARG_BYTEA_P(0);
	path = PG_GETARG_ARRAYTYPE_P(1);

	bsize = VARSIZE(ptr) - VARHDRSZ;
	data = (const uint8_t *)VARDATA(ptr);

	if (array_contains_nulls(path)) {
		elog(ERROR, "Invalid path data");
		PG_RETURN_NULL();
	}

	if (!data_is_cbor(data, bsize)) {
		PG_RETURN_NULL();
	}

	deconstruct_array(path, TEXTOID, -1, false, 'i', &pathtext, &pathnulls, &npath);

	if (CborIteratorInit(&iter, data, bsize)) {
		if (npath <= 0) {
			CborIteratorNext(&iter);
			if (CborIteratorGetType(&iter) == CborTypeFloat) {
				ret = CborIteratorGetFloat(&iter);
				CborIteratorFinalize(&iter);
				PG_RETURN_FLOAT8(ret);
			}
			CborIteratorFinalize(&iter);
		} else if (PgCborIteratorPath(&iter, pathtext, npath)) {
			if (CborIteratorGetType(&iter) == CborTypeFloat) {
				ret = CborIteratorGetFloat(&iter);
				CborIteratorFinalize(&iter);
				PG_RETURN_FLOAT8(ret);
			}
			CborIteratorFinalize(&iter);
		}
	}
	PG_RETURN_NULL();
}

Datum
cbor_path_as_bool(PG_FUNCTION_ARGS) {
	bytea *ptr;
	ArrayType *path;

	size_t bsize;
	const uint8_t *data;

	Datum *pathtext;
	bool *pathnulls;
	int	npath;

	CborIteratorContext iter;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	ptr = PG_GETARG_BYTEA_P(0);
	path = PG_GETARG_ARRAYTYPE_P(1);

	bsize = VARSIZE(ptr) - VARHDRSZ;
	data = (const uint8_t *)VARDATA(ptr);

	if (array_contains_nulls(path)) {
		elog(ERROR, "Invalid path data");
		PG_RETURN_NULL();
	}

	if (!data_is_cbor(data, bsize)) {
		PG_RETURN_NULL();
	}

	deconstruct_array(path, TEXTOID, -1, false, 'i', &pathtext, &pathnulls, &npath);

	if (CborIteratorInit(&iter, data, bsize)) {
		if (npath <= 0) {
			CborIteratorNext(&iter);
			if (CborIteratorGetType(&iter) == CborTypeTrue) {
				CborIteratorFinalize(&iter);
				PG_RETURN_BOOL(true);
			} else if (CborIteratorGetType(&iter) == CborTypeFalse) {
				CborIteratorFinalize(&iter);
				PG_RETURN_BOOL(false);
			}
			CborIteratorFinalize(&iter);
		} else if (PgCborIteratorPath(&iter, pathtext, npath)) {
			if (CborIteratorGetType(&iter) == CborTypeTrue) {
				CborIteratorFinalize(&iter);
				PG_RETURN_BOOL(true);
			} else if (CborIteratorGetType(&iter) == CborTypeFalse) {
				CborIteratorFinalize(&iter);
				PG_RETURN_BOOL(false);
			}
			CborIteratorFinalize(&iter);
		}
	}
	PG_RETURN_NULL();
}

/*


to_jsonb(anyelement)
	Returns the value as json or jsonb. Arrays and composites are converted (recursively) to arrays and objects; otherwise, if there is a cast from the type to json, the cast function will be used to perform the conversion; otherwise, a scalar value is produced. For any scalar type other than a number, a Boolean, or a null value, the text representation will be used, in such a fashion that it is a valid json or jsonb value.

array_to_json(anyarray [, pretty_bool])
	Returns the array as a JSON array. A PostgreSQL multidimensional array becomes a JSON array of arrays. Line feeds will be added between dimension-1 elements if pretty_bool is true.

row_to_json(record [, pretty_bool])
	Returns the row as a JSON object. Line feeds will be added between level-1 elements if pretty_bool is true.

jsonb_build_array(VARIADIC "any")
	Builds a possibly-heterogeneously-typed JSON array out of a variadic argument list.

jsonb_build_object(VARIADIC "any")
	Builds a JSON object out of a variadic argument list. By convention, the argument list consists of alternating keys and values.

jsonb_object(text[])
	Builds a JSON object out of a text array. The array must have either exactly one dimension with an even number of members, in which case they are taken as alternating key/value pairs, or two dimensions such that each inner array has exactly two elements, which are taken as a key/value pair.

jsonb_object(keys text[], values text[])
	This form of json_object takes keys and values pairwise from two separate arrays. In all other respects it is identical to the one-argument form.





json_array_length(json)
jsonb_array_length(jsonb)
	int 	Returns the number of elements in the outermost JSON array.

json_each(json)
jsonb_each(jsonb)
	Expands the outermost JSON object into a set of key/value pairs.

json_each_text(json)
jsonb_each_text(jsonb)
	Expands the outermost JSON object into a set of key/value pairs. The returned values will be of type text.

json_extract_path(from_json json, VARIADIC path_elems text[])
jsonb_extract_path(from_json jsonb, VARIADIC path_elems text[])
	Returns JSON value pointed to by path_elems (equivalent to #> operator).

json_extract_path_text(from_json json, VARIADIC path_elems text[])
jsonb_extract_path_text(from_json jsonb, VARIADIC path_elems text[])
	text 	Returns JSON value pointed to by path_elems as text (equivalent to #>> operator).

json_object_keys(json)
jsonb_object_keys(jsonb)
	setof text 	Returns set of keys in the outermost JSON object.

json_populate_record(base anyelement, from_json json)
jsonb_populate_record(base anyelement, from_json jsonb)
	anyelement 	Expands the object in from_json to a row whose columns match the record type defined by base (see note below).

json_populate_recordset(base anyelement, from_json json)
jsonb_populate_recordset(base anyelement, from_json jsonb)
	setof anyelement 	Expands the outermost array of objects in from_json to a set of rows whose columns match the record type defined by base (see note below).

json_array_elements(json)
jsonb_array_elements(jsonb)
	Expands a JSON array to a set of JSON values.

json_array_elements_text(json)
jsonb_array_elements_text(jsonb)
	setof text 	Expands a JSON array to a set of text values.

json_typeof(json)
jsonb_typeof(jsonb)
	text 	Returns the type of the outermost JSON value as a text string. Possible types are object, array, string, number, boolean, and null.

json_to_record(json)
jsonb_to_record(jsonb)
	record 	Builds an arbitrary record from a JSON object (see note below). As with all functions returning record, the caller must explicitly define the structure of the record with an AS clause.

json_to_recordset(json)
jsonb_to_recordset(jsonb)
	setof record 	Builds an arbitrary set of records from a JSON array of objects (see note below). As with all functions returning record, the caller must explicitly define the structure of the record with an AS clause.

json_strip_nulls(from_json json)
jsonb_strip_nulls(from_json jsonb)
	Returns from_json with all object fields that have null values omitted. Other null values are untouched.

jsonb_set(target jsonb, path text[], new_value jsonb[, create_missing boolean])
	Returns target with the section designated by path replaced by new_value, or with new_value added if create_missing is true ( default is true) and the item designated by path does not exist. As with the path orientated operators, negative integers that appear in path count from the end of JSON arrays.

jsonb_insert(target jsonb, path text[], new_value jsonb, [insert_after boolean])
	Returns target with new_value inserted. If target section designated by path is in a JSONB array, new_value will be inserted before target or after if insert_after is true (default is false). If target section designated by path is in JSONB object, new_value will be inserted only if target does not exist. As with the path orientated operators, negative integers that appear in path count from the end of JSON arrays.

jsonb_pretty(from_json jsonb)
	text Returns from_json as indented JSON text.


-> 	int 	Get JSON array element (indexed from zero, negative integers count from the end)
-> 	text 	Get JSON object field by key
->> 	int 	Get JSON array element as text
->> 	text 	Get JSON object field as text
#> 	text[] 	Get JSON object at specified path
#>> 	text[] 	Get JSON object at specified path as text


@> 	jsonb 	Does the left JSON value contain the right JSON path/value entries at the top level?
<@ 	jsonb 	Are the left JSON path/value entries contained at the top level within the right JSON value?
? 	text 	Does the string exist as a top-level key within the JSON value?
?| 	text[] 	Do any of these array strings exist as top-level keys?
?& 	text[] 	Do all of these array strings exist as top-level keys?
|| 	jsonb 	Concatenate two jsonb values into a new jsonb value
- 	text 	Delete key/value pair or string element from left operand. Key/value pairs are matched based on their key value.
- 	integer 	Delete the array element with specified index (Negative integers count from the end). Throws an error if top level container is not an array.
#- 	text[] 	Delete the field or element with specified path (for JSON arrays, negative integers count from the end)

*/
