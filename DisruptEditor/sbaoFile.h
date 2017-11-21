#pragma once

#include <stdint.h>
#include <SDL_rwops.h>
#include "Vector.h"

struct sbaoLayer {
	Vector<uint8_t> data;

	//Cached Data
	int channels, sampleRate, samples;
	void fillCache();
	void replace(const char* filename);

	enum Type { VORBIS, PCM };
	Type type;
	int play(bool loop);
};

class sbaoFile {
public:
	void open(const char* filename);
	void open(SDL_RWops* fp);
	void save(const char* filename);
	Vector<sbaoLayer> layers;
private:
	void fillCache();
};

