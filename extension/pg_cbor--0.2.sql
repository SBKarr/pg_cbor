CREATE OR REPLACE FUNCTION public.is_cbor(bytea)
	RETURNS boolean AS
	'pg_cbor.so', 'is_cbor'
	LANGUAGE c IMMUTABLE LEAKPROOF;

CREATE OR REPLACE FUNCTION public.cbor_to_string(bytea)
	RETURNS text AS
	'pg_cbor.so', 'cbor_to_string'
	LANGUAGE c IMMUTABLE LEAKPROOF;

CREATE OR REPLACE FUNCTION public.cbor_extract_path(bytea, VARIADIC text[])
	RETURNS bytea AS
	'pg_cbor.so', 'cbor_extract_path'
	LANGUAGE c IMMUTABLE LEAKPROOF;

CREATE OR REPLACE FUNCTION public.cbor_extract_path_text(bytea, VARIADIC text[])
	RETURNS text AS
	'pg_cbor.so', 'cbor_extract_path_text'
	LANGUAGE c IMMUTABLE LEAKPROOF;

CREATE OR REPLACE FUNCTION public.cbor_path_as_text(bytea, VARIADIC text[])
	RETURNS text AS
	'pg_cbor.so', 'cbor_path_as_text'
	LANGUAGE c IMMUTABLE LEAKPROOF;

CREATE OR REPLACE FUNCTION public.cbor_path_as_bytes(bytea, VARIADIC text[])
	RETURNS bytea AS
	'pg_cbor.so', 'cbor_path_as_bytes'
	LANGUAGE c IMMUTABLE LEAKPROOF;

CREATE OR REPLACE FUNCTION public.cbor_path_as_int(bytea, VARIADIC text[])
	RETURNS bigint AS
	'pg_cbor.so', 'cbor_path_as_int'
	LANGUAGE c IMMUTABLE LEAKPROOF;

CREATE OR REPLACE FUNCTION public.cbor_path_as_float(bytea, VARIADIC text[])
	RETURNS double precision AS
	'pg_cbor.so', 'cbor_path_as_float'
	LANGUAGE c IMMUTABLE LEAKPROOF;
	
CREATE OR REPLACE FUNCTION public.cbor_path_as_bool(bytea, VARIADIC text[])
	RETURNS boolean AS
	'pg_cbor.so', 'cbor_path_as_bool'
	LANGUAGE c IMMUTABLE LEAKPROOF;
	