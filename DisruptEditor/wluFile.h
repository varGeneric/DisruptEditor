#pragma once

#include <stdint.h>

struct baseHeader {
	char magic[4];
	uint32_t size;
	uint32_t unknown1;
	uint32_t unknown2;
};

struct fcbHeader {
	char magic[4];
	uint16_t version;
	uint16_t headerFlags;
	uint32_t totalObjectCount;
	uint32_t totalValueCount;
};

struct wluHeader {
	baseHeader base;
	fcbHeader fcb;
};

class wluFile {
public:
	wluFile() {};
	bool open(const char* filename);


};

