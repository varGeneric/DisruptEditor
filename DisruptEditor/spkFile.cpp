#include "spkFile.h"

#include <stddef.h>
#include <stdint.h>
#include <SDL_assert.h>
#include <SDL_log.h>
#include "Vector.h"
#include "Common.h"

const uint32_t spkMagic = 1397771010;

#pragma pack(push, 1)
struct spkHeader {
	uint32_t magic = spkMagic;
	uint32_t type = 0;
	uint32_t unk2 = 0;
	uint32_t unk3 = 0;
	uint32_t unk4 = 0;
	uint32_t rawSize = 0;
};
#pragma pack(pop)

void spkFile::open(const char * filename) {
	if (!filename) return;
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	if (!fp) return;
	spkHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == spkMagic);

	if (head.type == 1) {
		//Embedded Elsewhere
		type = REFERENCE;
		SDL_assert_release(false);
	} else if (head.type == 2) {
		type = REFERENCE;

		SDL_RWseek(fp, 292, RW_SEEK_CUR);
		uint32_t file = SDL_ReadLE32(fp);
		char buffer[500];
		snprintf(buffer, sizeof(buffer), "soundbinary\\%08x.sbao", file);
		sbao.open(getAbsoluteFilePath(buffer).c_str());

	} else if (head.type == 3) {
		type = EMBEDDED;
		sbao.open(fp);

		//SDL_assert_release(head.unk2 + 2 == head.unk3 + 1 == head.unk4);

		size_t offset = SDL_RWtell(fp);
		SDL_assert_release(offset - 360 == head.rawSize);
	} else if (head.type == 9) {
		SDL_assert_release(false);
	} else {
		SDL_assert_release(false);
	}

	SDL_RWclose(fp);
}
