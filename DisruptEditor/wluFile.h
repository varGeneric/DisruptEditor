#pragma once

#include <stdint.h>
#include "tinyxml2.h"
#include <string>
#include "Vector.h"
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
	bool open(std::string filename);

	void serialize(const char* filename);

	void draw(bool drawImgui = false, bool draw3D = false);

	bool bailOut;//If the file could possibly be corrupt
	
	Node root;

	std::string shortName; //ex. wlu_data_01_loop_vigilante_01.xml.data.fcb
	std::string origFilename; //ex. original filename opened with
private:
	wluHeader wluhead;
	Vector<uint8_t> extraData;
	void handleHeaders(SDL_RWops *fp, size_t size);
	Node* selectedEntity = NULL;
};

