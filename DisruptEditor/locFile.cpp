#include "locFile.h"

#include <algorithm>
#include <SDL_assert.h>
#include <SDL_rwops.h>
#include <SDL_log.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include "Vector.h"

#pragma pack(push, 1)
struct locHeader {
	uint32_t magic;
	uint16_t langCode;
	uint16_t count;
	uint32_t fragmentOffset;
};
struct locEntry {
	uint32_t hash;
	uint32_t offset;
};
struct locTableEntry {
	uint16_t rightIndex = 0;
	uint16_t leftIndex = 0;
	std::string resolve(const Vector<locTableEntry> &list) const;
};
#pragma pack(pop)

bool locFile::open(const char *filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");

	locHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == 85075);
	SDL_assert_release(head.fragmentOffset <= SDL_RWsize(fp));

	SDL_RWseek(fp, head.fragmentOffset, RW_SEEK_SET);

	Vector<std::string> strings;
	uint32_t maxStringFragmentIndex;
	{
		uint32_t numStringFrags = SDL_ReadLE32(fp);
		maxStringFragmentIndex = std::min(254u, numStringFrags);
		int stringFragmentIndexMask = (int)numStringFrags * 255;
		Vector<locTableEntry> stringFragments(numStringFrags);
		SDL_RWread(fp, stringFragments.data() + 1, sizeof(locTableEntry), stringFragments.size() - 1);
		SDL_assert_release(SDL_RWtell(fp) == SDL_RWsize(fp));

		// resolve list
		strings.reserve(numStringFrags);
		for (int i = 0; i < numStringFrags; i++) {
			std::string str = stringFragments[i].resolve(stringFragments);
			strings.push_back(str);
			//SDL_Log("%s\n", str.c_str());
		}
	}

	SDL_RWseek(fp, 12 + 8 * head.count, RW_SEEK_SET);
	int consumed = 0;
	int tryConsumingThatManyBytes = 600;
	std::string str;
	while (consumed < tryConsumingThatManyBytes) {
		uint8_t b = SDL_ReadU8(fp);
		++consumed;

		if (b == 0) {
			str += "\n";
		} else if (b < maxStringFragmentIndex) {
			str += strings[b + 1];
		} else {
			str += "\nUnknown\n";
		}
	}
	SDL_Log("%s\n", str.c_str());

	SDL_RWclose(fp);
	return true;
}

std::string locTableEntry::resolve(const Vector<locTableEntry> &list) const {
	// linked list index == 0
	if (leftIndex == 0 && rightIndex == 0)
		return "";

	// is character
	if (leftIndex == 0)
		return std::string(1, *(char*)&rightIndex);

	// is linked list element
	std::string ret;
	if (leftIndex >= 0 && leftIndex < list.size()) {
		ret += list[leftIndex].resolve(list);
	}
	if (rightIndex >= 0 && rightIndex < list.size()) {
		ret += list[rightIndex].resolve(list);
	}

	return ret;
}
