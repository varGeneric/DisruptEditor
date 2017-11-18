#pragma once

#include <stdint.h>
#include <SDL_rwops.h>
#include "Vector.h"

struct sbaoLayer {
	Vector<uint8_t> data;
	enum Type { VORBIS, PCM };
	Type type;
	int play(bool loop);
};

class sbaoFile {
public:
	void open(const char* filename);
	void open(SDL_RWops* fp);
	Vector<sbaoLayer> layers;
};

