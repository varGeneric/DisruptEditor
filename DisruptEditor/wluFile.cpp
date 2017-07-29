#include "wluFile.h"

#include <stdio.h>
#include <string.h>

#include "BinaryObject.h"

bool wluFile::open(const char * filename) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		return false;
	}

	wluHeader wluhead;
	fread(&wluhead, sizeof(wluhead), 1, fp);
	if (memcmp(wluhead.magic, "ESAB", 4) != 0) {
		fclose(fp);
		return false;
	}

	fcbHeader fcbhead;
	fread(&fcbhead, sizeof(fcbhead), 1, fp);

	std::vector<std::shared_ptr<BinaryObject>> pointers;
	std::shared_ptr<BinaryObject> root = Deserialize(NULL, fp, pointers);

	fclose(fp);
	return true;
}