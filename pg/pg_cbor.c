
#include "pg_cbor.h"
#include "utils/builtins.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(is_cbor);
PG_FUNCTION_INFO_V1(cbor_to_string);

Datum
is_cbor(PG_FUNCTION_ARGS) {
	bytea *ptr;
	size_t bsize;
	const uint8_t *data;

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
	StringInfo str;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	ptr = PG_GETARG_BYTEA_P(0);
	bsize = VARSIZE(ptr) - VARHDRSZ;

	data = (const uint8_t *)VARDATA(ptr);
	if (data_is_cbor(data, bsize)) {
		str = makeStringInfo();
		CborToString(str, data, bsize);
		PG_RETURN_TEXT_P(cstring_to_text_with_len(str->data, str->len));
	} else {
		PG_RETURN_NULL();
	}
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
