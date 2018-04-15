#include "Common.h"

#include "Vector.h"
#include "Hash.h"
#include <unordered_map>
#include <SDL_assert.h>
#include <Shlwapi.h>
#include <stdio.h>
#include <SDL_log.h>
#include "NBCF.h"
#include "xbgFile.h"
#include "materialFile.h"
#include "xbtFile.h"
#include "tinyfiles.h"
#include "stb_image.h"

Settings settings;
std::unordered_map<std::string, materialFile> materials;
std::unordered_map<std::string, xbtFile> textures;
std::unordered_map<uint32_t, std::string> knownFiles;

void reloadSettings() {
	tinyxml2::XMLDocument doc;
	doc.LoadFile("settings.xml");

	//Search Path
	settings.searchPaths.clear();

	if (doc.RootElement()->FirstChildElement("patchDir")) {
		settings.patchDir = doc.RootElement()->FirstChildElement("patchDir")->Attribute("src");
		if (settings.patchDir.back() != '/' && settings.patchDir.back() != '\\')
			settings.patchDir.push_back('/');
		settings.searchPaths.push_back(settings.patchDir);
	}

	for (auto it = doc.RootElement()->FirstChildElement("PackFolder"); it; it = it->NextSiblingElement("PackFolder")) {
		std::string packPath = it->Attribute("src");
		if (packPath.back() != '/' && packPath.back() != '\\')
			packPath.push_back('/');
		settings.searchPaths.push_back(packPath);
	}

	if (doc.RootElement()->FirstChildElement("textDrawDistance"))
		settings.textDrawDistance = doc.RootElement()->FirstChildElement("textDrawDistance")->FloatAttribute("src");

	if (doc.RootElement()->FirstChildElement("drawBuildings"))
		settings.drawBuildings = doc.RootElement()->FirstChildElement("drawBuildings")->BoolAttribute("src");

	//Reload Filelist
	knownFiles.clear();
	FILE* fp = fopen("res/Watch Dogs.filelist", "r");
	char buffer[500];
	while(fgets(buffer, sizeof(buffer), fp)) {
		buffer[strlen(buffer)-1] = '\0';
		knownFiles[Hash::instance().getFilenameHash(buffer)] = buffer;
	}
	fclose(fp);
}

template <typename T>
void pushXMLSetting(tinyxml2::XMLPrinter &printer, const char *name, const T &value) {
	printer.OpenElement(name);
	printer.PushAttribute("src", value);
	printer.CloseElement();
}

template <>
void pushXMLSetting<std::string>(tinyxml2::XMLPrinter &printer, const char *name, const std::string &value) {
	printer.OpenElement(name);
	printer.PushAttribute("src", value.c_str());
	printer.CloseElement();
}

#define pushXMLSetting(p, x) pushXMLSetting(p, #x, settings.x)

void saveSettings() {
	FILE *fp = fopen("settings.xml", "w");
	tinyxml2::XMLPrinter printer(fp);
	printer.OpenElement("Settings");

	for (const std::string &base : settings.searchPaths) {
		if (base == settings.patchDir) continue;

		printer.OpenElement("PackFolder");
		printer.PushAttribute("src", base.c_str());
		printer.CloseElement();
	}

	pushXMLSetting(printer, patchDir);
	pushXMLSetting(printer, textDrawDistance);
	pushXMLSetting(printer, drawBuildings);

	printer.CloseElement();
	fclose(fp);
}

Vector<FileInfo> getFileList(const std::string &dir, const std::string &extFilter) {
	std::map<std::string, tfFILE> files;

	for (const std::string &base : settings.searchPaths) {
		std::string fullPath = base + dir;
		if (!PathFileExistsA(fullPath.c_str())) continue;

		tfDIR dir;
		tfDirOpen(&dir, fullPath.c_str());
		while (dir.has_next) {
			tfFILE file;
			tfReadFile(&dir, &file);

			if (!file.is_dir && files.count(file.name) == 0) {
				if(extFilter.empty() || file.ext == extFilter)
					files[file.name] = file;
			}

			tfDirNext(&dir);
		}
		tfDirClose(&dir);

	}

	Vector<FileInfo> outFiles;
	for (auto &file : files) {
		FileInfo fi;
		fi.name = file.second.name;
		fi.fullPath = file.second.path;
		fi.ext = file.second.ext;
		outFiles.push_back(fi);
	}
	return outFiles;
}

Vector<FileInfo> getFileListFromAbsDir(const std::string & fullDir, const std::string & extFilter) {
	Vector<FileInfo> outFiles;
	if (!PathFileExistsA(fullDir.c_str())) return outFiles;

	tfDIR dir;
	tfDirOpen(&dir, fullDir.c_str());
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir) {
			if (extFilter.empty() || file.ext == extFilter) {
				FileInfo fi;
				fi.name = file.name;
				fi.fullPath = file.path;
				fi.ext = file.ext;
				outFiles.push_back(fi);
			}
		}

		tfDirNext(&dir);
	}
	tfDirClose(&dir);

	return outFiles;
}

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
	for (const std::string &base : settings.searchPaths) {
		std::string fullPath = base + path;
		if (PathFileExistsA(fullPath.c_str()))
			return fullPath;
	}

	uint32_t hash = Hash::instance().getFilenameHash(path);
	//if (unknownFiles.count(hash))
		//return unknownFiles[hash];

	return std::string();
}

std::string getAbsoluteFilePath(uint32_t path) {
	//if (unknownFiles.count(path))
		//return unknownFiles[path];
	if (knownFiles.count(path))
		return getAbsoluteFilePath(knownFiles[path]);
	return "";
}

SDL_RWops * openFile(const std::string & path) {
	//FILE *oFP = fopen(getAbsoluteFilePath(path).c_str(), "rb");

	SDL_RWops *fp = SDL_RWFromFile(getAbsoluteFilePath(path).c_str(), "rb");
	return fp;
}

SDL_RWops * openFile(uint32_t path) {
	FILE *oFP = fopen(getAbsoluteFilePath(path).c_str(), "rb");

	SDL_RWops *fp = SDL_RWFromFP(oFP, SDL_TRUE);
	return fp;
}

std::unordered_map<uint32_t, xbgFile> xbgs;

xbgFile& loadXBG(const std::string &path) {
	return loadXBG(Hash::instance().getFilenameHash(path));
}

xbgFile &loadXBG(uint32_t path) {
	if (xbgs.count(path) == 0) {
		auto &model = xbgs[path];
		SDL_Log("Loading %u.xbg\n", path);
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

std::unordered_map<std::string, GLuint> texturesRes;
GLuint loadResTexture(const std::string & path) {
	if (texturesRes.count(path) == 0) {
		int width, height, bpc;
		uint8_t *pixels = stbi_load(("res/" + path).c_str(), &width, &height, &bpc, 0);
		if (!pixels) return 0;
		GLuint id;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		switch (bpc) {
			case 4:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
				break;
			case 3:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
				break;
			case 2:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, pixels);
				break;
			case 1:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
				break;
		}
		glGenerateMipmap(GL_TEXTURE_2D);
		texturesRes[path] = id;
		free(pixels);
		return id;
	}
	return texturesRes[path];
}

