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
#include "tinyfiles.h"

Settings settings;
std::unordered_map<std::string, xbgFile> xbgs;
std::unordered_map<std::string, materialFile> materials;
std::unordered_map<std::string, xbtFile> textures;

void reloadSettings() {
	tinyxml2::XMLDocument doc;
	doc.LoadFile("settings.xml");

	//Search Path
	settings.searchPaths.clear();
	for (auto it = doc.RootElement()->FirstChildElement("PackFolder"); it; it = it->NextSiblingElement("PackFolder")) {
		std::string packPath = it->Attribute("src");
		if (packPath.back() != '/' && packPath.back() != '\\')
			packPath.push_back('/');
		settings.searchPaths.push_back(packPath);
	}
	
	if (doc.RootElement()->FirstChildElement("patchDir"))
		settings.patchDir = doc.RootElement()->FirstChildElement("patchDir")->Attribute("src");

	if (doc.RootElement()->FirstChildElement("textDrawDistance"))
		settings.textDrawDistance = doc.RootElement()->FirstChildElement("textDrawDistance")->FloatAttribute("src");

	if (doc.RootElement()->FirstChildElement("drawBuildings"))
		settings.drawBuildings = doc.RootElement()->FirstChildElement("drawBuildings")->BoolAttribute("src");
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

	return std::string();
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

std::map<std::string, Node> entityLibrary;
std::unordered_map<uint32_t, std::string> entityLibraryUID;

void addEntity(uint32_t UID, Node node) {
	Attribute *hidName = node.getAttribute("hidName");
	SDL_assert_release(hidName);

	std::string name = (char*)hidName->buffer.data();
	entityLibrary[name] = node;
	entityLibraryUID[UID] = name;
}

Node* findEntityByUID(uint32_t UID) {
	if (entityLibraryUID.count(UID) > 0)
		return &entityLibrary[entityLibraryUID[UID]];
	return NULL;
}

