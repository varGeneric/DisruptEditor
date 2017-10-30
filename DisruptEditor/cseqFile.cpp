#include "cseqFile.h"

#include <SDL_assert.h>

#pragma pack(push, 1)
struct cseqHeader {
	uint64_t magic;
	uint8_t unk2[12];
};
#pragma pack(pop)

bool cseqFile::open(const char *filename) {
	FILE *fp = fopen(filename, "rb");
	cseqHeader head;
	fread(&head, sizeof(head), 1, fp);
	SDL_assert_release(head.magic == 23438637261664779);

	fcbHeader fcbHead;
	fread(&fcbHead, sizeof(fcbHead), 1, fp);

	Vector<Node*> list;
	root.deserializeA(fp, list);

	fclose(fp);
	return true;
}
