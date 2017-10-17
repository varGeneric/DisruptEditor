#pragma once

#include "NBCF.h"

class cseqFile {
public:
	bool open(const char* filename);

	Node root;
};

