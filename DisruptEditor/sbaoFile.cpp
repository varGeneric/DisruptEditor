#include "sbaoFile.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <vector>

#include <vorbis/vorbisfile.h>

struct spkHeader {
	uint32_t magic;
	uint8_t unk[24];
};

const uint32_t sbaoMagic = 207362;

void sbaoFile::open(const char * filename) {
	FILE *fp = fopen(filename, "rb");
	spkHeader head;
	fread(&head, sizeof(head), 1, fp);
	assert(head.magic == sbaoMagic);

	char oggs[4];
	fread(oggs, 1, 4, fp);
	assert(memcmp(oggs, "OggS", 4) == 0);

	/*std::vector<uint32_t> unk(head.numPackets);
	fread(unk.data(), sizeof(uint32_t), head.numPackets, fp);

	//Skip 32 bytes
	fseek(fp, 32, SEEK_CUR);

	//Read Oggs
	char oggs[4];
	fread(oggs, 1, 4, fp);
	assert(memcmp(oggs, "OggS", 4) == 0);*/

	fclose(fp);
}

