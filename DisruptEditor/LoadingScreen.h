#pragma once

#include <stdint.h>

struct SDL_Window;
struct SDL_Thread;
struct SDL_Renderer;
struct SDL_Texture;
struct stbtt_bakedchar;
typedef uint32_t SDL_AudioDeviceID;

#include <string>
#include <mutex>
#include "Vector.h"

class LoadingScreen {
public:
	LoadingScreen();
	~LoadingScreen();
	void threadHandler();
	std::string title, message;
	std::mutex mutex;
	void setTitle(const std::string &title, const std::string &message = std::string(), float percentage = -1.f);
	void setProgress(const std::string &message, float percentage = -1.f);
	SDL_Window* getWindow() { return window; }
private:
	float percentage;
	short *audioData;
	int channels, audioSize;
	Vector<stbtt_bakedchar> cdata;
	SDL_Thread *thread;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* background, *fontTexture;
	SDL_AudioDeviceID dev = 0;
	bool running = true;
	void drawText(float x, float y, const char* str);
	float calcTextLength(const char* str);
};

