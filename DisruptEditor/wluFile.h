#pragma once

#include <stdint.h>
#include "tinyxml2.h"
#include <string>
#include <vector>
#include "NBCF.h"

struct baseHeader {
	char magic[4];
	uint32_t size;
	uint32_t unknown1;//0 or 1 or 2 or 3
	uint32_t unknown2;//0
};

struct wluHeader {
	baseHeader base;
	fcbHeader fcb;
};

struct qualityHeader {
	char magic[4];
	uint16_t size;
	uint8_t unknown[10];
};

struct roadHeader {
	char magic[4];
	uint32_t size;
	uint8_t unknown[8];
};

class wluFile {
public:
	wluFile() {};
	bool open(const char* filename);

	void handleHeaders(FILE *fp, size_t size);

	void serialize(FILE *fp);

	wluHeader wluhead;
	Node root;
	std::string origFilename;
	std::string shortName;
};

