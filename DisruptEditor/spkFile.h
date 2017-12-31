#pragma once

#include "sbaoFile.h"

class spkFile {
public:
	void open(const char* filename);
	void save(const char* filename);

	enum Type { EMBEDDED, REFERENCE };
	Type type;

	sbaoFile sbao;

	uint32_t file;
};

