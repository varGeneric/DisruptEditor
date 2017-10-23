#include "xbtFile.h"

#include <fstream>

bool xbtFile::open(const char *file) {
	std::ifstream fs(file, std::ios::binary);

	//Seek past xbt header
	fs.seekg(8, std::ios_base::cur);
	int32_t offset;
	fs.read((char*)&offset, sizeof(offset));
	fs.seekg(offset, std::ios_base::beg);

	image.load(fs);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	image.upload_texture2D();

	fs.close();

	return true;
}
