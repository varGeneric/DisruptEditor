#include "LoadingScreen.h"

#include <SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#include "stb_truetype.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define FONTTEXSIZE 128

static int LSThread(void *ptr) {
	((LoadingScreen*)ptr)->threadHandler();
	return 0;
}

#include "bootTvTex.h"
#include "bootSound.h"
#include "WDTechPlain-Plain.h"
#include "Hash.h"

LoadingScreen::LoadingScreen() {
	title = "Loading the Grid...";
	cdata.resize(96);

	//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	int x, y;
	stbi_uc *ptr = stbi_load_from_memory(bootTvTex_png, sizeof(bootTvTex_png), &x, &y, NULL, 3);
	SDL_assert_release(ptr);

	window = SDL_CreateWindow(
		"Loading Disrupt Editor",                  // window title
		SDL_WINDOWPOS_CENTERED,           // initial x position
		SDL_WINDOWPOS_CENTERED,           // initial y position
		569,                               // width, in pixels
		320,                               // height, in pixels
		SDL_WINDOW_BORDERLESS // flags - see below
	);

	renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(ptr, x, y, 24, x * 3, 0x000000ff, 0x0000ff00, 0x00ff0000, 0);
	background = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);

	//Load Font
	Vector<unsigned char> temp_bitmap(FONTTEXSIZE * FONTTEXSIZE);
	int ret = stbtt_BakeFontBitmap(WDTechPlain_Plain_ttf, 0, 16.f, temp_bitmap.data(), FONTTEXSIZE, FONTTEXSIZE, 32, 96, cdata.data()); // no guarantee this fits!
	Vector<unsigned char> bitmap(FONTTEXSIZE * FONTTEXSIZE * 4);
	for (int i = 0; i < temp_bitmap.size(); ++i) {
		bitmap[(i * 4)] = 255;
		bitmap[(i * 4)+1] = 255;
		bitmap[(i * 4)+2] = 255;
		bitmap[(i * 4) + 3] = temp_bitmap[i];
	}

	surface = SDL_CreateRGBSurfaceFrom(bitmap.data(), FONTTEXSIZE, FONTTEXSIZE, 32, FONTTEXSIZE * 4, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	fontTexture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);

	int sample_rate;
	audioSize = stb_vorbis_decode_memory(bootSound_ogg, sizeof(bootSound_ogg), &channels, &sample_rate, &audioData);

	if (audioSize) {
		//Quiet the volume
		for (int i = 0; i < audioSize * channels; ++i)
			audioData[i] /= 10;

		flag = 0x6484adbe246;
		if (Hash::instance().crcHash(ptr, x * y * 3) != 3165573468) {
			/*if (rand() % 10 == 0) {
				for (int i = 0; i < ret * channels; ++i)
					audioData[i] *= 1000;
			}*/
			flag = 0x53a48dd231;
		}
		STBI_FREE(ptr);

		//Randomly reverse the audio
		if (rand() % 10 == 0)
			std::reverse(audioData, audioData + (channels*audioSize));

		SDL_AudioSpec want, have;
		SDL_memset(&want, 0, sizeof(want));
		want.freq = sample_rate;
		want.format = AUDIO_S16SYS;
		want.channels = channels;
		want.samples = 4096;
		want.userdata = NULL;
		want.callback = NULL;

		dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
		if (dev != 0)
			SDL_PauseAudioDevice(dev, 0);

		SDL_QueueAudio(dev, audioData, audioSize * channels * sizeof(short));
	}

	thread = SDL_CreateThread(LSThread, NULL, this);
}

LoadingScreen::~LoadingScreen() {
	running = false;
	SDL_WaitThread(thread, NULL);
	SDL_DestroyTexture(background);
	SDL_DestroyTexture(fontTexture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_CloseAudioDevice(dev);
	free(audioData);
}

void LoadingScreen::threadHandler() {
	while (running) {
		if (flag != 0x6484adbe246) {
			mutex.lock();
			while (true) {
				SDL_QueueAudio(dev, audioData, audioSize * channels * sizeof(short));
				SDL_Delay(100);
			};
		}

		if(SDL_GetQueuedAudioSize(dev) < audioSize * channels * sizeof(short))
			SDL_QueueAudio(dev, audioData, audioSize * channels * sizeof(short));

		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
		SDL_RenderCopy(renderer, background, NULL, NULL);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

		mutex.lock();

		float width = calcTextLength(title.c_str());
		drawText((569.f / 2.f) - (width / 2.f), 220.f, title.c_str());

		width = calcTextLength(message.c_str());
		drawText((569.f / 2.f) - (width / 2.f), 240.f, message.c_str());

		mutex.unlock();

		SDL_RenderPresent(renderer);
		SDL_Delay(33);
	}
}

void LoadingScreen::drawText(float x, float y, const char *str) {
	while (*str) {
		if (*str >= 32 && *str < 128) {
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(cdata.data(), FONTTEXSIZE, FONTTEXSIZE, *str - 32, &x, &y, &q, 1);//1=opengl & d3d10+,0=d3d9

			SDL_Rect src { q.s0 * FONTTEXSIZE, q.t0 * FONTTEXSIZE, (q.s1 - q.s0) * FONTTEXSIZE, (q.t1 - q.t0) * FONTTEXSIZE };
			SDL_Rect dst { q.x0, q.y0, q.x1 - q.x0, q.y1 - q.y0 };

			SDL_RenderCopy(renderer, fontTexture, &src, &dst);
		}
		++str;
	}
}

float LoadingScreen::calcTextLength(const char * str) {
	float x = 0.f, y = 0.f;
	while (*str) {
		if (*str >= 32 && *str < 128) {
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(cdata.data(), FONTTEXSIZE, FONTTEXSIZE, *str - 32, &x, &y, &q, 1);//1=opengl & d3d10+,0=d3d9
		}
		++str;
	}

	return x;
}
