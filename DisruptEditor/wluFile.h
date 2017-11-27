#pragma once

#include <stdint.h>
#include "tinyxml2.h"
#include <string>
#include "Vector.h"
#include "NBCF.h"

#pragma pack(push, 1)
struct wluHeader {
	uint32_t magic;
	uint32_t size;
	uint32_t unknown1;//0 or 1 or 2 or 3
	uint32_t unknown2;//0
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
#pragma pack(pop)

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
	bool openWD1(SDL_RWops *fp);
	bool openWD2(SDL_RWops *fp);

	bool isWD2 = false;
	wluHeader wluhead;
	Vector<uint8_t> extraData;
	void handleHeaders(SDL_RWops *fp, size_t size);
	Node* selectedEntity = NULL;
};

