#include "CSector.h"

#include <stdio.h>
#include <stdint.h>
#include <SDL_assert.h>
#include <SDL_rwops.h>
#include "Vector.h"

struct CSectorHighResHeader {
	uint32_t magic;
	uint32_t unk1;
	uint32_t unk2;
	uint32_t unk3;
	uint32_t unk4;
	uint32_t unk5;
	uint32_t unk6;
	uint32_t unk7;
};

bool CSectorHighRes::open(const char *filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	if (!fp) {
		return false;
	}
	Vector<uint8_t> data(SDL_RWsize(fp));
	SDL_RWread(fp, data.data(), data.size(), 1);
	SDL_RWclose(fp);
	fp = SDL_RWFromConstMem(data.data(), data.size());

	CSectorHighResHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == 1397901394);
	SDL_assert_release(head.unk1 == 4);
	SDL_assert_release(head.unk2 == 1 || head.unk2 == 16);
	SDL_assert_release(head.unk3 == 252645135);
	//SDL_assert_release(head.unk4 == 0);
	//SDL_assert_release(head.unk5 == 0);
	//SDL_assert_release(head.unk6 == 0);
	//SDL_assert_release(head.unk7 == 0);

	SDL_RWclose(fp);
	return true;
}
