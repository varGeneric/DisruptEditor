#pragma once

#include <stdint.h>
#include <mutex>
#include <SDL_audio.h>
#include <map>
#include "Vector.h"

struct PlayingSound {
	PlayingSound() {}
	PlayingSound(int sampleRate, int channels, const short *input, int inputLen, bool loops);
	int readSound(float* out, int len);
	void destroy();
	SDL_AudioStream *stream = NULL;
	Vector<short> data;
	bool loop;
	bool done = false;
	float volume = 1.f;
};

class Audio {
public:
	Audio();
	~Audio();
	static Audio& instance();
	void audioCallback(float* stream, int len);
	std::mutex mutex;
	int addSound(int sampleRate, int channels, const short *input, int inputLen, bool loops);
	void stopSound(int id);
private:
	SDL_AudioDeviceID dev;
	int count = 1;
	std::map<int, PlayingSound> sounds;
};

