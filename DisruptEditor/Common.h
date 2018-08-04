#pragma once

#include "Vector.h"
#include <string>
#include <map>
#include <unordered_map>
#include "glm/glm.hpp"
#include "glad.h"
#include <SDL_rwops.h>
#include <SDL_scancode.h>
#include <SDL_keycode.h>

class xbgFile;
class materialFile;
class xbtFile;
class Node;

struct Settings {
	Vector<std::string> searchPaths;
	std::string patchDir;
	float textDrawDistance = 5.f;
	bool drawBuildings = true;

	// Camera controls
	SDL_Scancode keyForward = SDL_SCANCODE_W;
	SDL_Scancode keyBackward = SDL_SCANCODE_S;
	SDL_Scancode keyLeft = SDL_SCANCODE_A;
	SDL_Scancode keyRight = SDL_SCANCODE_D;
	SDL_Scancode keyAscend = SDL_SCANCODE_SPACE;
	SDL_Scancode keyDescend = SDL_SCANCODE_LALT;
	SDL_Keymod keyFast = KMOD_LSHIFT;
	SDL_Keymod keySlow = KMOD_LCTRL;
};

extern Settings settings;

void reloadSettings();
void saveSettings();

struct FileInfo {
	std::string fullPath;
	std::string name;
	std::string ext;
};
Vector<FileInfo> getFileList(const std::string &dir, const std::string &extFilter = std::string());
Vector<FileInfo> getFileListFromAbsDir(const std::string &dir, const std::string &extFilter = std::string());

std::string loadFile(const std::string &file);

std::string getAbsoluteFilePath(const std::string &path);
std::string getAbsoluteFilePath(uint32_t path);

SDL_RWops* openFile(const std::string &path);
SDL_RWops* openFile(uint32_t path);

xbgFile& loadXBG(const std::string &path);
xbgFile& loadXBG(uint32_t path);

materialFile& loadMaterial(const std::string &path);

xbtFile& loadTexture(const std::string &path);

GLuint loadResTexture(const std::string &path);

#include "imgui.h"

namespace ImGui {
	static bool InputUInt64(const char * label, uint64_t * v) {
		std::string a = std::to_string(*v);
		char temp[36];
		strncpy(temp, a.c_str(), sizeof(temp));

		if (ImGui::InputText(label, temp, sizeof(temp), ImGuiInputTextFlags_CharsDecimal)) {
			*v = std::stoull(temp);
			return true;
		}

		return false;
	}
};
