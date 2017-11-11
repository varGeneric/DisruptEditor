#include "locFile.h"

#include <algorithm>
#include <SDL_assert.h>
#include <SDL_rwops.h>
#include <SDL_log.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include "Vector.h"
#include <SDL_stdinc.h>

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
	std::wstring resolve(const Vector<locTableEntry> &list) const;
};
#pragma pack(pop)

std::string ConvertUTF16ToUTF8(const std::wstring pszTextUTF16) {
	if (pszTextUTF16.empty()) return std::string();

	char* dst = SDL_iconv_string("UTF-8", "UTF-16", (const char*)pszTextUTF16.c_str(), pszTextUTF16.size() * 2 + 2);
	std::string str(dst);
	SDL_free(dst);
	return str;
}

bool locFile::open(const char *filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");

	locHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == 85075);
	SDL_assert_release(head.fragmentOffset <= SDL_RWsize(fp));

	SDL_RWseek(fp, head.fragmentOffset, RW_SEEK_SET);

	Vector<std::wstring> strings;
	uint32_t numStringFrags;
	uint32_t maxStringFragmentIndex;
	{
		numStringFrags = SDL_ReadLE32(fp);
		maxStringFragmentIndex = std::min(254u, numStringFrags);
		int stringFragmentIndexMask = (int)numStringFrags * 255;
		Vector<locTableEntry> stringFragments(numStringFrags);
		SDL_RWread(fp, stringFragments.data() + 1, sizeof(locTableEntry), stringFragments.size() - 1);
		SDL_assert_release(SDL_RWtell(fp) == SDL_RWsize(fp));

		// resolve list
		strings.reserve(numStringFrags);
		for (int i = 0; i < numStringFrags; i++) {
			strings.push_back(stringFragments[i].resolve(stringFragments));
		}
	}

	SDL_RWseek(fp, 12 + 8 * head.count, RW_SEEK_SET);
	int consumed = 0;
	int tryConsumingThatManyBytes = 600;
	std::wstring str;
	Vector<std::wstring> compiledStrings;
	while (SDL_RWtell(fp) < head.fragmentOffset) {
		uint8_t b = SDL_ReadU8(fp);
		++consumed;

		if (b == 0) {
			compiledStrings.push_back(str);
			SDL_Log("%s\n", ConvertUTF16ToUTF8(str).c_str());
			str.clear();
		} else if (b < maxStringFragmentIndex) {
			str += strings[b + 1];
		} else {
			str += L"*Unknown*";
		}
	}

	size_t a = compiledStrings.size();

	SDL_RWclose(fp);
	return true;
}

std::wstring locTableEntry::resolve(const Vector<locTableEntry> &list) const {
	// linked list index == 0
	if (leftIndex == 0 && rightIndex == 0)
		return std::wstring();

	// is character
	if (leftIndex == 0)
		return std::wstring(1, rightIndex);

	// is linked list element
	SDL_assert_release(leftIndex < list.size());
	SDL_assert_release(rightIndex < list.size());

	return list[leftIndex].resolve(list) + list[rightIndex].resolve(list);
}
