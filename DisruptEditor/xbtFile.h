#pragma once

#include "nv_dds.h"

class xbtFile {
public:
	bool open(const char* file);
	nv_dds::CDDSImage image;
	GLuint id;
};

