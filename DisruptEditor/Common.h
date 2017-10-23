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
