
#ifndef INCLUDE_CBOR_DATA_H_
#define INCLUDE_CBOR_DATA_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct CborData {
	uint32_t size;
	const uint8_t * ptr;
} CborData;

extern uint32_t CborHeaderSize;
extern const uint8_t CborHeaderData[];

bool data_is_cbor(const uint8_t *, uint32_t);

static inline bool CborDataIsEmpty(const CborData *data) {
	return data->size > 0;
}

static inline bool CborDataOffset(CborData *data, uint32_t size) {
	if (data->size > size) {
		data->ptr += size;
		data->size -= size;
		return true;
	}

	data->size = 0;
	return false;
}

uint8_t CborDataGetUnsigned(const CborData *);
uint16_t CborDataGetUnsigned16(const CborData *);
uint32_t CborDataGetUnsigned32(const CborData *);
uint64_t CborDataGetUnsigned64(const CborData *);
float CborDataGetFloat16(const CborData *);
float CborDataGetFloat32(const CborData *);
double CborDataGetFloat64(const CborData *);
uint64_t CborDataGetUnsignedValue(const CborData *, uint8_t hint);

uint64_t CborDataReadUnsignedValue(CborData *, uint8_t hint);

#endif /* INCLUDE_CBOR_DATA_H_ */
