#include "Common.h"

#include "Vector.h"
#include <unordered_map>
#include <SDL_assert.h>
#include <Shlwapi.h>
#include <SDL_log.h>
#include "NBCF.h"
#include "xbgFile.h"
#include "materialFile.h"
#include "xbtFile.h"

Vector<std::string> searchPaths;
std::unordered_map<std::string, xbgFile> xbgs;
std::unordered_map<std::string, materialFile> materials;
std::unordered_map<std::string, xbtFile> textures;

std::string loadFile(const std::string & file) {
	FILE* fp = fopen(file.c_str(), "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		size_t size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char data[1024 * 5];
		SDL_assert_release(size < sizeof(data) - 1);
		fread(data, 1, size, fp);
		data[size] = '\0';
		fclose(fp);
		return data;
	}
	return std::string();
}

std::string getAbsoluteFilePath(const std::string &path) {
	for (const std::string &base : searchPaths) {
		std::string fullPath = base + path;
		if (PathFileExistsA(fullPath.c_str()))
			return fullPath;
	}

	return std::string();
}

void addSearchPath(const std::string &path) {
	searchPaths.push_back(path);
}

xbgFile& loadXBG(const std::string &path) {
	if (xbgs.count(path) == 0) {
		auto &model = xbgs[path];
		SDL_Log("Loading %s...\n", path.c_str());
		model.open(getAbsoluteFilePath(path).c_str());
	}
	return xbgs[path];
}

materialFile &loadMaterial(const std::string & path) {
	if (materials.count(path) == 0) {
		auto &model = materials[path];
		SDL_Log("Loading %s...\n", path.c_str());
		model.open(getAbsoluteFilePath(path).c_str());
	}
	return materials[path];
}

xbtFile & loadTexture(const std::string & path) {
	if (textures.count(path) == 0) {
		auto &model = textures[path];
		SDL_Log("Loading %s...\n", path.c_str());
		model.open(getAbsoluteFilePath(path).c_str());
	}
	return textures[path];
}

std::unordered_map<uint32_t, Node> entityLibrary;

