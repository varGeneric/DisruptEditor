#include "CSector.h"

#include <stdio.h>
#include <stdint.h>
#include <SDL_assert.h>

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
	FILE *fp = fopen(filename, "rb");

	CSectorHighResHeader head;
	fread(&head, sizeof(head), 1, fp);
	SDL_assert_release(head.magic == 1397901394);
	SDL_assert_release(head.unk1 == 4);
	SDL_assert_release(head.unk2 == 1 || head.unk2 == 16);
	SDL_assert_release(head.unk3 == 252645135);
	//SDL_assert_release(head.unk4 == 0);
	//SDL_assert_release(head.unk5 == 0);
	//SDL_assert_release(head.unk6 == 0);
	//SDL_assert_release(head.unk7 == 0);

	fclose(fp);
	return true;
}
