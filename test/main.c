
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <ftw.h>
#include <dirent.h>

#include "cbor_iter.h"

static void print_hex(const uint8_t *s, size_t size) {
	while (size > 0) {
		printf("%02x", *s++);
		-- size;
	}
}

static void process_cbor_value(const CborIteratorContext *iter, const char *token) {
	switch (CborIteratorGetType(iter)) {
	case CborTypeUnsigned: printf("%s Unsigned(%lu)\n", token, CborIteratorGetUnsigned(iter)); break;
	case CborTypeNegative: printf("%s Negative(%ld)\n", token, CborIteratorGetInteger(iter)); break;
	case CborTypeByteString:
		printf("%s ByteString(", token);
		print_hex(CborIteratorGetBytePtr(iter), CborIteratorGetObjectSize(iter));
		printf(")\n");
		break;
	case CborTypeCharString: printf("%s CharString(%.*s)\n", token, CborIteratorGetObjectSize(iter), CborIteratorGetCharPtr(iter)); break;
	case CborTypeArray: printf("%s Array\n", token); break;
	case CborTypeMap: printf("%s Map\n", token); break;
	case CborTypeTag: printf("%s Tag(%lu)\n", token, CborIteratorGetUnsigned(iter)); break;
	case CborTypeSimple: printf("%s Simple(%lu)\n", token, CborIteratorGetUnsigned(iter)); break;
	case CborTypeFloat: printf("%s Float(%.10e)\n", token, CborIteratorGetFloat(iter)); break;
	case CborTypeFalse: printf("%s False)\n", token); break;
	case CborTypeTrue: printf("%s True\n", token); break;
	case CborTypeNull: printf("%s Null\n", token); break;
	case CborTypeUndefined: printf("%s Undefined\n", token); break;
	}
}

static void do_indent(uint32_t l) {
	while (l > 0) {
		printf("\t");
		-- l;
	}
}

void process_file_data(const char *filename, const uint8_t *data, size_t size) {
	uint32_t level = 1;
	CborIteratorContext iter;
	printf("File: %s\n", filename);
	if (CborIteratorInit(&iter, data, size)) {
		CborIteratorToken token = CborIteratorNext(&iter);
		while (token != CborIteratorTokenDone) {
			switch (token) {
			case CborIteratorTokenDone:
				do_indent(level);
				printf("Done\n");
				break;
			case CborIteratorTokenKey:
				do_indent(level);
				process_cbor_value(&iter, "Key");
				break;
			case CborIteratorTokenValue:
				do_indent(level);
				process_cbor_value(&iter, "Value");
				break;
			case CborIteratorTokenBeginArray:
				do_indent(level);
				printf("BeginArray (%u)\n", CborIteratorGetContainerSize(&iter));
				++ level;
				break;
			case CborIteratorTokenEndArray:
				-- level;
				do_indent(level);
				printf("EndArray\n");
				break;
			case CborIteratorTokenBeginObject:
				do_indent(level);
				printf("BeginObject (%u)\n", CborIteratorGetContainerSize(&iter));
				++ level;
				break;
			case CborIteratorTokenEndObject:
				-- level;
				do_indent(level);
				printf("EndObject\n");
				break;
			case CborIteratorTokenBeginByteStrings:
				do_indent(level);
				printf("BeginByteStrings\n");
				++ level;
				break;
			case CborIteratorTokenEndByteStrings:
				-- level;
				do_indent(level);
				printf("EndByteStrings\n");
				break;
			case CborIteratorTokenBeginCharStrings:
				do_indent(level);
				printf("BeginCharStrings\n");
				++ level;
				break;
			case CborIteratorTokenEndCharStrings:
				-- level;
				do_indent(level);
				printf("EndCharStrings\n");
				break;
			}
			token = CborIteratorNext(&iter);
		}

		CborIteratorFinalize(&iter);
	}
}

void read_file(const char *dirname, const char *filename) {
	char buf[PATH_MAX + 1] = { 0 };

	FILE *fp;
	sprintf(buf, "%s/%s", dirname, filename);
	fp = fopen(buf, "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		long int size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		uint8_t buf[size];
		if (fread(buf, size, 1, fp) == 1) {
			process_file_data(filename, buf, size);
		}

		fclose(fp);
	}
}

void print_file(const char *dirname, const char *filename) {
	char buf[PATH_MAX + 1] = { 0 };

	FILE *fp;
	sprintf(buf, "%s/%s", dirname, filename);
	fp = fopen(buf, "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		long int size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char buf[size + 1];
		memset(buf, 0, size + 1);
		if (fread(buf, size, 1, fp) == 1) {
			printf("File: %s\n\t%s\n", filename, buf);
		}

		fclose(fp);
	}
}

int main(int argc, char* argv[]) {
	char buf[PATH_MAX + 1] = { 0 };

	char *cwd = getcwd(buf, PATH_MAX);
	strcat(cwd, "/test/data");

	if (argc > 1) {
		cwd = realpath(argv[1], buf);
	}

	char nameBuf[PATH_MAX + 1] = { 0 };
	for (uint32_t i = 1; i < 79; ++ i) {
		sprintf(nameBuf, "%u.json", i);
		print_file(cwd, nameBuf);
		sprintf(nameBuf, "%u.diag", i);
		print_file(cwd, nameBuf);
		sprintf(nameBuf, "%u.cbor", i);
		read_file(cwd, nameBuf);
	}

	/*

	DIR *dir;
	struct dirent *dp;
	dir = opendir(cwd);
	while ((dp = readdir(dir)) != NULL) {
		const char *name = dp->d_name;
		size_t len = strlen(name);
		if (len > 5 && memcmp(".cbor", name + len - 5, 5) == 0) {
			read_file(cwd, name);
		}
	}
	closedir(dir);*/

	return 0;
}
