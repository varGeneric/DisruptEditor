#include "batchFile.h"

#include <SDL_assert.h>
#include <SDL_log.h>
#include <SDL_rwops.h>
#include <string.h>
#include <string>

#pragma pack(push, 1)
struct batchHeader {
	uint32_t magic;
	uint32_t unk1; //32
	uint32_t type; //0 for compound, 1 for phys
	uint32_t unk2;
	uint32_t unk3;
	uint32_t unk4;//0
	uint32_t unk5;//0
	uint32_t unk6;//0
	uint32_t unk7;//0
	uint32_t unk8;//0
	uint32_t unk9;//0
	uint32_t unk10;//0
};
struct compoundHeader {
	uint32_t unk1;
	uint32_t unk2;
	uint32_t unk3;
	uint32_t unk4;
	uint32_t unk5;
};
#pragma pack(pop)

static inline void seekpad(SDL_RWops *fp, long pad) {
	//16-byte chunk alignment
	long size = SDL_RWtell(fp);
	long seek = (pad - (size % pad)) % pad;
	SDL_RWseek(fp, seek, RW_SEEK_CUR);
}

std::string readString(SDL_RWops *fp, bool bigEndian) {
	uint32_t len = bigEndian ? SDL_ReadBE32(fp) : SDL_ReadLE32(fp);

	std::string str(len + 1, '\0');
	SDL_RWread(fp, &str[0], 1, len);
	return str;
}

bool batchFile::open(const char * filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	if (!fp) return false;

	bool bigEndian = false;
	size_t size = SDL_RWsize(fp);

	batchHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == 1112818504);
	SDL_assert_release(head.unk1 == 32);
	SDL_assert_release(head.type == 0 || head.type == 1);
	SDL_assert_release(head.unk2 < SDL_RWsize(fp));
	SDL_assert_release(head.unk4 == 0);
	SDL_assert_release(head.unk5 == 0);
	SDL_assert_release(head.unk6 == 0);
	//SDL_assert_release(head.unk7 == 0);
	//SDL_assert_release(head.unk8 == 0);
	//SDL_assert_release(head.unk9 == 0);
	//SDL_assert_release(head.unk10 == 0);

	if (head.type == 0) {
		SDL_assert_release(strstr(filename, "_compound.cbatch"));

		compoundHeader compound;
		SDL_RWread(fp, &compound, sizeof(compound), 1);
		std::string srcFilename = readString(fp, bigEndian);
		seekpad(fp, 4);

		SDL_Log("%s\n", srcFilename.c_str());
	} else if (head.type == 1) {
		SDL_assert_release(strstr(filename, "_phys.cbatch"));
	}

	SDL_RWclose(fp);
	return true;
}
