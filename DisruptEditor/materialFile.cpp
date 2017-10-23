#include "materialFile.h"

#include <stdio.h>
#include <assert.h>

static inline void seekpad(FILE *fp, long pad) {
	//16-byte chunk alignment
	long size = ftell(fp);
	long seek = (pad - (size % pad)) % pad;
	fseek(fp, seek, SEEK_CUR);
}

bool materialFile::open(const char *filename) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) return false;

	matHeader head;
	fread(&head, sizeof(head), 1, fp);

	assert(head.magic == 5062996);
	assert(head.unknum == 7);
	assert(head.unk2[0] == 0);
	assert(head.unk2[1] == 0);
	assert(head.unk2[2] == 0);
	assert(head.unk3[0] == 0);
	assert(head.unk3[1] == 0);
	assert(head.unk4 == 0);
	assert(head.unk5 == 0);
	assert(head.unk6 == 0);

	assert(head.size == head.size3);
	assert(head.size == head.size4);

	while (!feof(fp)) {
		matEntry me;

		uint32_t size;
		fread(&size, sizeof(size), 1, fp);
		me.name.resize(size+1, '\0');
		fread(&me.name[0], 1, size, fp);
		seekpad(fp, 4);

		fread(&size, sizeof(size), 1, fp);
		me.shader.resize(size + 1, '\0');
		fread(&me.shader[0], 1, size, fp);
		seekpad(fp, 4);

		//Skip 28 bytes and guess
		fseek(fp, 28, SEEK_CUR);

		fread(&size, sizeof(size), 1, fp);
		if (size != 4183327151) {
			fclose(fp);
			return false;
		}

		fread(&size, sizeof(size), 1, fp);
		me.texture.resize(size + 1, '\0');
		fread(&me.texture[0], 1, size, fp);
		seekpad(fp, 4);

		printf("%s\n", me.name.c_str());
		printf("%s\n", me.shader.c_str());
		printf("%s\n", me.texture.c_str());

		entries.push_back(me);

		fclose(fp);
		return true;
	}

	fclose(fp);
	return true;
}
