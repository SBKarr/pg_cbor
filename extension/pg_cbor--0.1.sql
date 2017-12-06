CREATE OR REPLACE FUNCTION public.is_cbor(bytea)
	RETURNS boolean AS
	'pg_cbor.so', 'is_cbor'
	LANGUAGE c IMMUTABLE LEAKPROOF;

CREATE OR REPLACE FUNCTION public.cbor_to_string(bytea)
	RETURNS text AS
	'pg_cbor.so', 'cbor_to_string'
	LANGUAGE c IMMUTABLE LEAKPROOF;