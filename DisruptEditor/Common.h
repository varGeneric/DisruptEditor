#pragma once

#include "Vector.h"
#include <string>
#include <map>
#include <unordered_map>
#include "glm/glm.hpp"
#include "glad.h"

class xbgFile;
class materialFile;
class xbtFile;
class Node;

struct Settings {
	Vector<std::string> searchPaths;
	std::string patchDir;
	float textDrawDistance = 5.f;
	bool drawBuildings = true;
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
