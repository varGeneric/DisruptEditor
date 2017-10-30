#include "spkFile.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <SDL_assert.h>
#include "Vector.h"

#include <vorbis/vorbisfile.h>

struct spkHeader {
	uint32_t magic;
	uint32_t numPackets;
};

const uint32_t spkMagic = 1397771010;

void spkFile::open(const char * filename) {
	FILE *fp = fopen(filename, "rb");
	spkHeader head;
	fread(&head, sizeof(head), 1, fp);
	SDL_assert_release(head.magic == spkMagic);

	printf("%i\n", head.numPackets);

	/*Vector<uint32_t> unk(head.numPackets);
	fread(unk.data(), sizeof(uint32_t), head.numPackets, fp);

	//Skip 32 bytes
	fseek(fp, 32, SEEK_CUR);

	//Read Oggs
	char oggs[4];
	fread(oggs, 1, 4, fp);
	SDL_assert_release(memcmp(oggs, "OggS", 4) == 0);*/

	fclose(fp);
}
