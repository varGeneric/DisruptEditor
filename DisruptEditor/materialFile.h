#pragma once

#include <stdint.h>
#include "Vector.h"
#include <string>

struct matHeader {
	uint32_t magic; // 54 41 4D 00
	uint32_t unknum; //7
	uint32_t unk[3];
	uint32_t unk2[3]; //00
	uint32_t size;
	uint32_t size2;
	uint32_t unk3[2]; //00
	uint32_t size3; //Repeat of size
	uint32_t unk4; //00
	uint32_t size4; //Repeat of size
	uint32_t unk5; //00
	uint32_t unk6; //00
};

struct matEntry {
	std::string name;
	std::string shader;
	std::string texture;
};

class materialFile {
public:
	bool open(const char *filename);
	Vector< matEntry > entries;
};

