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

	fcbHeader fcbHead;
	SDL_RWread(fp, &fcbHead, sizeof(fcbHead), 1);

	Vector<Node*> list;
	root.deserializeA(fp, list);

	SDL_RWclose(fp);
	return true;
}
