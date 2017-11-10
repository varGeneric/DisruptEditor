#include "locFile.h"

#include <SDL_assert.h>
#include <SDL_rwops.h>
#include <stdio.h>
#include <stdint.h>
#include "Vector.h"

#pragma pack(push, 1)
struct locHeader {
	uint32_t magic;
	uint8_t pad8[4];
	uint32_t size;
};
struct locEntry {
	uint32_t hash;
	uint32_t offset;
};

#pragma pack(pop)

bool locFile::open(const char *filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");

	locHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == 85075);
	SDL_assert_release(head.size <= SDL_RWsize(fp));

	Vector<uint8_t> data(head.size - sizeof(head));
	SDL_RWread(fp, data.data(), data.size(), 1);

	SDL_RWclose(fp);
	return true;
}
