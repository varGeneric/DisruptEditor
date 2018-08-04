#pragma once

#include "sbaoFile.h"

class spkFile {
public:
	void open(const char* filename);
	void save(const char* filename);

	Vector<sbaoFile> objs;

	uint32_t file;
};

