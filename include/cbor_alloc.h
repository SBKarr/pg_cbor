
#ifndef INCLUDE_CBOR_ALLOC_H_
#define INCLUDE_CBOR_ALLOC_H_

#include <stdlib.h>

void *CborAlloc(size_t);
void *CborRealloc(void *, size_t);
void CborFree(void *);

#endif /* INCLUDE_CBOR_ALLOC_H_ */
