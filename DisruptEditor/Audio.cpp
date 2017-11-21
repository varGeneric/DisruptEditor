#include "Audio.h"

#include <SDL_audio.h>

void CAudioCallback(void* userdata, uint8_t* stream, int len) {
	Audio::instance().audioCallback((float*)stream, len);
}

Audio::Audio() {
	SDL_AudioSpec want, have;
	SDL_memset(&want, 0, sizeof(want));
	want.freq = 48000;
	want.format = AUDIO_F32SYS;
	want.channels = 2;
	want.samples = 4096;
	want.userdata = NULL;
	want.callback = CAudioCallback;

	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (dev != 0)
		SDL_PauseAudioDevice(dev, 0);
}

Audio::~Audio() {
	SDL_CloseAudioDevice(dev);
}

Audio& Audio::instance() {
	static Audio a;
	return a;
}

void Audio::audioCallback(float *stream, int len) {
	memset(stream, 0, len);
	int sampleLen = len / sizeof(float);
	Vector<float> temp(sampleLen);
	mutex.lock();
	for (auto it = sounds.begin(); it != sounds.end();) {
		int readLen = it->second.readSound(temp.data(), len);
		int sampleReadLen = readLen / sizeof(float);

		for (int i = 0; i < sampleReadLen; ++i)
			stream[i] += temp[i] * it->second.volume;

		if (it->second.done) {
			it->second.destroy();
			it = sounds.erase(it);
			continue;
		}

		++it;
	}
	mutex.unlock();
}

int Audio::addSound(int sampleRate, int channels, const short * input, int inputLen, bool loops) {
	mutex.lock();
	int id = count++;
	sounds[id] = PlayingSound(sampleRate, channels, input, inputLen, loops);
	mutex.unlock();

	return id;
}

void Audio::stopSound(int id) {
	mutex.lock();
	sounds[id].destroy();
	sounds.erase(id);
	mutex.unlock();
}

void Audio::stopAll() {
	mutex.lock();
	for (auto it = sounds.begin(); it != sounds.end(); ++it)
		it->second.destroy();
	sounds.clear();
	mutex.unlock();
}

PlayingSound::PlayingSound(int sampleRate, int channels, const short *input, int inputLen, bool loops) {
	loop = loops;
	data.assign(input, input + (inputLen / sizeof(short)));
	stream = SDL_NewAudioStream(AUDIO_S16SYS, channels, sampleRate, AUDIO_F32SYS, 2, 48000);
	SDL_AudioStreamPut(stream, data.data(), data.size() * sizeof(short));
	if (!loop)
		SDL_AudioStreamFlush(stream);
}

int PlayingSound::readSound(float *out, int len) {
	if (!stream) {
		memset(out, 0, len);
		return 0;
	}

	if (SDL_AudioStreamAvailable(stream) < len) {
		if(loop)
			SDL_AudioStreamPut(stream, data.data(), data.size() * sizeof(short));
		else {
			memset(out, 0, len);
			done = true;
			return SDL_AudioStreamGet(stream, out, len);
		}
	}

	int got = SDL_AudioStreamGet(stream, out, len);
	SDL_assert_release(got == len);
	return got;
}

void PlayingSound::destroy() {
	if(stream)
		SDL_FreeAudioStream(stream);
}
