#include "Common.h"

#include <vector>
#include <assert.h>

std::vector<std::string> searchPaths;

std::string loadFile(const std::string & file) {
	FILE* fp = fopen(file.c_str(), "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		size_t size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char data[1024 * 5];
		assert(size < sizeof(data) - 1);
		fread(data, 1, size, fp);
		data[size] = '\0';
		fclose(fp);
		return data;
	}
	return std::string();
}

std::string getAbsoluteFilePath(const std::string &path) {
	return std::string();
}

void addSearchPath(const std::string &path) {
	searchPaths.push_back(path);
}
