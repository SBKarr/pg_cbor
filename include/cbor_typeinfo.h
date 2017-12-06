
#ifndef INCLUDE_CBOR_TYPEINFO_H_
#define INCLUDE_CBOR_TYPEINFO_H_

typedef enum {
	CborMajorTypeUnsigned	= 0,
	CborMajorTypeNegative	= 1,
	CborMajorTypeByteString	= 2,
	CborMajorTypeCharString	= 3,
	CborMajorTypeArray		= 4,
	CborMajorTypeMap		= 5,
	CborMajorTypeTag		= 6,
	CborMajorTypeSimple		= 7
} CborMajorType;

typedef enum {
	CborSimpleValueFalse		= 20,
	CborSimpleValueTrue			= 21,
	CborSimpleValueNull			= 22,
	CborSimpleValueUndefined	= 23,
} CborSimpleValue;

typedef enum {
	CborFlagsNull = 0,
	CborFlagsInterrupt = 0xFF,

	CborFlagsMajorTypeShift = 5, // bitshift for major type id

	CborFlagsMajorTypeMask = 7, // mask for major type id
	CborFlagsMajorTypeMaskEncoded = 7 << CborFlagsMajorTypeShift, // mask for major type id

	CborFlagsAdditionalInfoMask = 31, // mask for additional info

	CborFlagsMaxAdditionalNumber = 24,
	CborFlagsAdditionalNumber8Bit = 24,
	CborFlagsAdditionalNumber16Bit = 25,
	CborFlagsAdditionalNumber32Bit = 26,
	CborFlagsAdditionalNumber64Bit = 27,

	CborFlagsSimple8Bit = 24,
	CborFlagsAdditionalFloat16Bit = 25,
	CborFlagsAdditionalFloat32Bit = 26,
	CborFlagsAdditionalFloat64Bit = 27,

	CborFlagsUnassigned1 = 28,
	CborFlagsUnassigned2 = 29,
	CborFlagsUnassigned3 = 30,

	CborFlagsUndefinedLength = 31,
} CborFlags;

typedef enum {
	CborMajorTypeEncodedUnsigned	= CborMajorTypeUnsigned << CborFlagsMajorTypeShift,
	CborMajorTypeEncodedNegative	= CborMajorTypeNegative << CborFlagsMajorTypeShift,
	CborMajorTypeEncodedByteString	= CborMajorTypeByteString << CborFlagsMajorTypeShift,
	CborMajorTypeEncodedCharString	= CborMajorTypeCharString << CborFlagsMajorTypeShift,
	CborMajorTypeEncodedArray		= CborMajorTypeArray << CborFlagsMajorTypeShift,
	CborMajorTypeEncodedMap			= CborMajorTypeMap << CborFlagsMajorTypeShift,
	CborMajorTypeEncodedTag			= CborMajorTypeTag << CborFlagsMajorTypeShift,
	CborMajorTypeEncodedSimple		= CborMajorTypeSimple << CborFlagsMajorTypeShift
} CborMajorTypeEncoded;

typedef enum {
	CborTagDateTime = 0,	// string - Standard date/time string
	CborTagEpochTime = 1,	// multiple - Epoch-based date/time
	CborTagPositiveBignum = 2, // byte string
	CborTagNegativeBignum = 3, // byte string
	CborTagDecimalFraction = 4, // array - Decimal fraction;
	CborTagBigFloat = 5, // array
	/* 6-20 - Unassigned */
	CborTagExpectedBase64Url = 21, // multiple - Expected conversion to base64url encoding;
	CborTagExpectedBase64 = 22, // multiple - Expected conversion to base64 encoding;
	CborTagExpectedBase16 = 23, // multiple - Expected conversion to base16 encoding;
	CborTagEmbeddedCbor = 24, // byte string - Encoded CBOR data item;

	CborTagStringReference = 25, // unsigned integer - reference the nth previously seen string
	// [http://cbor.schmorp.de/stringref][Marc_A._Lehmann]

	CborTagSerializedPerl = 26, // array - Serialised Perl object with classname and constructor arguments
	// [http://cbor.schmorp.de/perl-object][Marc_A._Lehmann]

	CborTagSerializedObject = 27, // array - Serialised language-independent object with type name and constructor arguments
	// [http://cbor.schmorp.de/generic-object][Marc_A._Lehmann]

	CborTagSharedValue = 28, // multiple - mark value as (potentially) shared
	// [http://cbor.schmorp.de/value-sharing][Marc_A._Lehmann]

	CborTagValueReference = 29, // unsigned integer - reference nth marked value
	// [http://cbor.schmorp.de/value-sharing][Marc_A._Lehmann]

	CborTagRationalNumber = 30, // array - Rational number
	// [http://peteroupc.github.io/CBOR/rational.html][Peter_Occil]

	/* 31 - Unassigned */
	CborTagHintHintUri = 32, // UTF-8 string;
	CborTagHintBase64Url = 33, // UTF-8 string;
	CborTagHintBase64 = 34, // UTF-8 string;
	CborTagHintRegularExpression = 35, // UTF-8 string;
	CborTagHintMimeMessage = 36, // UTF-8 string;

	CborTagBinaryUuid = 37, // byte string - Binary UUID [RFC4122] section 4.1.2;
	// [https://github.com/lucas-clemente/cbor-specs/blob/master/uuid.md][Lucas_Clemente]

	CborTagLanguageTaggedString = 38, // byte string - [http://peteroupc.github.io/CBOR/langtags.html][Peter_Occil]
	CborTagIdentifier = 39, // multiple - [https://github.com/lucas-clemente/cbor-specs/blob/master/id.md][Lucas_Clemente]

	/* 40-255 - Unassigned */

	CborTagStringMark = 256, // multiple - mark value as having string references
	// [http://cbor.schmorp.de/stringref][Marc_A._Lehmann]

	CborTagBinaryMimeMessage = 257, // byte string - Binary MIME message
	// [http://peteroupc.github.io/CBOR/binarymime.html][Peter_Occil]

	/* 258-263 - Unassigned */
	CborTagDecimalFractionFixed = 264, // array - Decimal fraction with arbitrary exponent
	// [http://peteroupc.github.io/CBOR/bigfrac.html][Peter_Occil]

	CborTagBigFloatFixed = 265, // array - Bigfloat with arbitrary exponent
	// [http://peteroupc.github.io/CBOR/bigfrac.html][Peter_Occil]

	/* 266-22097 - Unassigned */
	CborTagHintIndirection = 22098, // multiple - hint that indicates an additional level of indirection
	// [http://cbor.schmorp.de/indirection][Marc_A._Lehmann]

	/* 22099-55798 - Unassigned */

	CborTagCborMagick = 55799, // multiple - Self-describe CBOR;
	/* 55800-18446744073709551615 - Unassigned */
} CborTag;

#endif /* INCLUDE_CBOR_TYPEINFO_H_ */
