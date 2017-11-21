#pragma once

#include "sbaoFile.h"

class spkFile {
public:
	void open(const char* filename);

	sbaoFile sbao;
};

