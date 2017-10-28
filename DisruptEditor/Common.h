#pragma once

#include <string>
#include "Math/Vector3.h"

class xbgFile;
class materialFile;
class xbtFile;

static inline vec3 swapYZ(const vec3 &ref) {
	return vec3(ref.x, ref.z, ref.y);
}

std::string loadFile(const std::string &file);

std::string getAbsoluteFilePath(const std::string &path);

void addSearchPath(const std::string &path);

xbgFile& loadXBG(const std::string &path);

materialFile& loadMaterial(const std::string &path);

xbtFile& loadTexture(const std::string &path);

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
