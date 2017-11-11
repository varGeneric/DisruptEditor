#pragma once

#include <stdint.h>
#include "Vector.h"

struct sbaoLayer {
	Vector<uint8_t> data;
	enum Type { VORBIS, PCM };
	Type type;
	void play(bool loop);
};

class sbaoFile {
public:
	void open(const char* filename);
	Vector<sbaoLayer> layers;
};

