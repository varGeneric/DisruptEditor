#include "batchFile.h"

#include <SDL_assert.h>
#include <SDL_rwops.h>

#pragma pack(push, 1)
struct batchHeader {
	uint32_t magic;
	uint32_t unk1; //32
	uint32_t unk2; //0
	uint32_t size;
	uint32_t unk3;
	uint32_t unk4;//0
	uint32_t unk5;//0
	uint32_t unk6;//0
};
#pragma pack(pop)

bool batchFile::open(const char * filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	if (!fp) return false;

	batchHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == 1112818504);
	SDL_assert_release(head.unk1 == 32);
	SDL_assert_release(head.unk2 == 0);
	SDL_assert_release(head.size < SDL_RWsize(fp));
	SDL_assert_release(head.unk4 == 0);
	SDL_assert_release(head.unk5 == 0);
	SDL_assert_release(head.unk6 == 0);

	SDL_RWclose(fp);
	return true;
}
