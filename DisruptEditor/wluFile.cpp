#include "wluFile.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "tinyxml2.h"
#include "Hash.h"

extern bool bailOut;

static inline void seekpad(FILE *fp, long pad) {
	//16-byte chunk alignment
	long size = ftell(fp);
	long seek = (pad - (size % pad)) % pad;
	fseek(fp, seek, SEEK_CUR);
}

static inline void writepad(FILE *fp, long pad) {
	//16-byte chunk alignment
	long size = ftell(fp);
	long seek = (pad - (size % pad)) % pad;

	char zero[32] = { 0 };
	fwrite(zero, 1, seek, fp);
}

bool wluFile::open(const char *filename) {
	origFilename.assign(filename);
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		return false;
	}

	wluHeader wluhead;
	fread(&wluhead, sizeof(wluhead), 1, fp);

	assert(memcmp(wluhead.base.magic, "ESAB", 4) == 0);
	assert(memcmp(wluhead.fcb.magic, "nbCF", 4) == 0);
	assert(wluhead.fcb.version == 16389);
	assert(wluhead.fcb.totalObjectCount == wluhead.fcb.totalValueCount + 1);

	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp) - sizeof(wluhead.base);
	fseek(fp, sizeof(wluhead), SEEK_SET);

	//Pad size to 4 bytes
	//TODO Figure out size
	//assert(wluhead.base.size == size || wluhead.base.size == size-1 || wluhead.base.size == size - 2 || wluhead.base.size == size - 3);

	//2296 size
	//2265 wlu base size + 16

	bailOut = false;
	root.deserialize(fp);

	fseek(fp, wluhead.base.size + sizeof(wluhead.base), SEEK_SET);
	seekpad(fp, 4);

	/*size_t offset = ftell(fp);
	if (offset != size + sizeof(wluhead.base)) {
		handleHeaders(fp, size + sizeof(wluhead.base));

		offset = ftell(fp);
		assert(offset == size + sizeof(wluhead.base));
	}*/

	fclose(fp);

	return !bailOut;
}

void wluFile::handleHeaders(FILE * fp, size_t size) {
	//Read Magic
	char magic[5];
	fread(magic, 4, 1, fp);
	magic[4] = '\0';
	fseek(fp, -4, SEEK_CUR);
	uint32_t magicNum = *(uint32_t*)(magic);

	//LAUQ - LoadQuality
	//ROAD - 

	if (magic == std::string("DAOR")) {
		roadHeader road;
		fread(&road, sizeof(road), 1, fp);
		fseek(fp, road.size, SEEK_CUR);
		printf("Road %i\n", road.size);
	} else if (magic == std::string("LAUQ")) {
		qualityHeader qual;
		fread(&qual, sizeof(qual), 1, fp);
		fseek(fp, qual.size, SEEK_CUR);
		printf("Qual %i\n", qual.size);
	} else if (magicNum == 1242679104) {
		fseek(fp, 4, SEEK_CUR);

		//Read Zeros
		uint8_t c;
		do {
			fread(&c, 1, 1, fp);
		} while (c == 0);
		fseek(fp, -1, SEEK_CUR);

		printf("What?\n");
	} else {
		size_t offset = ftell(fp);
		assert(false);
	}

	if (ftell(fp) != size) {
		handleHeaders(fp, size);
	}
}

void wluFile::serialize(FILE *fp) {
	wluHeader wluhead;

	memcpy(wluhead.base.magic, "ESAB", 4);
	wluhead.base.unknown1 = wluhead.base.unknown2 = 0;

	memcpy(wluhead.fcb.magic, "nbCF", 4);
	wluhead.fcb.version = 16389;
	wluhead.fcb.totalObjectCount = 1 + root.countNodes();
	wluhead.fcb.totalValueCount = wluhead.fcb.totalObjectCount - 1;

	fwrite(&wluhead, sizeof(wluhead), 1, fp);

	root.serialize(fp);

	wluhead.base.size = ftell(fp) - sizeof(wluhead.base);
	fseek(fp, 0, SEEK_SET);
	fwrite(&wluhead, sizeof(wluhead), 1, fp);

	fseek(fp, 0, SEEK_END);
	writepad(fp, 4);
}
