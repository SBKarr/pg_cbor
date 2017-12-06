

#ifdef POSTGRES

#include "pg_cbor.h"

void *CborAlloc(size_t bytes) {
	return palloc(bytes);
}

void *CborRealloc(void *ptr, size_t bytes) {
	return repalloc(ptr, bytes);
}

void CborFree(void *ptr) {
	pfree(ptr);
}

#else

#include "cbor_alloc.h"

void *CborAlloc(size_t bytes) {
	return malloc(bytes);
}

void *CborRealloc(void *ptr, size_t bytes) {
	return realloc(ptr, bytes);
}

void CborFree(void *ptr) {
	free(ptr);
}

#endif
