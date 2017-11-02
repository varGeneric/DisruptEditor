#include "locFile.h"

#include <SDL_assert.h>
#include <stdio.h>
#include <stdint.h>
#include "Vector.h"
#include <vector>

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
	FILE *fp = fopen(filename, "rb");

	locHeader head;
	fread(&head, sizeof(head), 1, fp);
	SDL_assert_release(head.magic == 85075);

	size_t count = (head.size - sizeof(head)) / sizeof(locEntry);
	std::vector<locEntry> entries(count);
	fread(entries.data(), sizeof(locEntry), count, fp);

	fclose(fp);
	return false;
}
