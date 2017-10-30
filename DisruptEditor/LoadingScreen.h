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
private:
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

