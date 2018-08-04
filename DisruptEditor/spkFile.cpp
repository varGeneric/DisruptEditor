#include "spkFile.h"

#include <stddef.h>
#include <stdint.h>
#include <SDL_assert.h>
#include <SDL_log.h>
#include "Vector.h"
#include "Common.h"

const uint32_t spkMagic = 1397771010;

static inline void seekpad(SDL_RWops *fp, long pad) {
	//16-byte chunk alignment
	long size = SDL_RWtell(fp);
	long seek = (pad - (size % pad)) % pad;
	SDL_RWseek(fp, seek, RW_SEEK_CUR);
}

static inline void writepad(SDL_RWops *fp, long pad) {
	//16-byte chunk alignment
	uint8_t buffer[16] = { 0 };
	long size = SDL_RWtell(fp);
	long seek = (pad - (size % pad)) % pad;
	SDL_RWwrite(fp, buffer, 1, seek);
}

void spkFile::open(const char * filename) {
	if (!filename) return;
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	if (!fp) return;
	uint32_t magic = SDL_ReadLE32(fp);
	SDL_assert_release(magic == spkMagic);

	uint32_t count = SDL_ReadLE32(fp);
	std::vector<uint32_t> soundIds(count);
	SDL_RWread(fp, soundIds.data(), sizeof(uint32_t), count);

	objs.resize(count);
	for (uint32_t i = 0; i < count; ++i) {
		uint32_t size = SDL_ReadLE32(fp);
		objs[i].open(fp);
		objs[i].id = soundIds[i];
		seekpad(fp, 4);
	}

	SDL_RWclose(fp);
}

void spkFile::save(const char * filename) {
	if (!filename) return;
	SDL_RWops *fp = SDL_RWFromFile(filename, "wb");
	if (!fp) return;
	SDL_WriteLE32(fp, spkMagic);
	SDL_WriteLE32(fp, objs.size());
	for (size_t i = 0; i < objs.size(); ++i)
		SDL_WriteLE32(fp, objs[i].id);

	for (size_t i = 0; i < objs.size(); ++i) {
		Vector<uint8_t> data = objs[i].save();
		SDL_WriteLE32(fp, data.size());
		SDL_RWwrite(fp, data.data(), 1, data.size());
		writepad(fp, 4);
	}

	SDL_RWclose(fp);
}
