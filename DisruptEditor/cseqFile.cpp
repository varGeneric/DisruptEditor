#include "cseqFile.h"

#include <SDL_assert.h>

#pragma pack(push, 1)
struct cseqHeader {
	uint64_t magic;
	uint8_t unk2[12];
};
#pragma pack(pop)

bool cseqFile::open(const char *filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	cseqHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == 23438637261664779);

	root = readFCB(fp);

	SDL_RWclose(fp);
	return true;
}
