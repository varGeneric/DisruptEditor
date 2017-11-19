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
struct locTableEntry {
	uint32_t idOffset;
	uint32_t offset;
};
struct locFragment {
	uint16_t rightIndex = 0;
	uint16_t leftIndex = 0;
	std::wstring resolve(const Vector<locFragment> &list) const;
};
#pragma pack(pop)

std::string ConvertUTF16ToUTF8(const std::wstring &pszTextUTF16) {
	if (pszTextUTF16.empty()) return std::string();

	char* dst = SDL_iconv_string("UTF-8", "UTF-16", (const char*)pszTextUTF16.c_str(), pszTextUTF16.size() * 2 + 2);
	std::string str(dst);
	SDL_free(dst);
	return str;
}

std::wstring ConvertUTF8ToUTF16(const std::string &pszTextUTF8) {
	if (pszTextUTF8.empty()) return std::wstring();

	char* dst = SDL_iconv_string("UTF-16", "UTF-8", pszTextUTF8.c_str(), pszTextUTF8.size() + 1);
	std::wstring str((wchar_t*)dst);
	SDL_free(dst);
	return str;
}

bool locFile::open(const char *filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");

	//Read Header
	locHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == 85075);
	SDL_assert_release(head.fragmentOffset <= SDL_RWsize(fp));

	//Read String Fragment Table
	SDL_RWseek(fp, head.fragmentOffset, RW_SEEK_SET);
	Vector<std::wstring> strings;
	uint32_t numStringFrags;
	int stringFragmentIndexMask;
	{
		numStringFrags = SDL_ReadLE32(fp);
		stringFragmentIndexMask = numStringFrags * 255;
		Vector<locFragment> stringFragments(numStringFrags);
		SDL_RWread(fp, stringFragments.data() + 1, sizeof(locFragment), stringFragments.size() - 1);
		SDL_assert_release(SDL_RWtell(fp) == SDL_RWsize(fp));

		//Resolve String Fragment Table
		strings.reserve(numStringFrags);
		for (int i = 0; i < numStringFrags; i++) {
			strings.push_back(stringFragments[i].resolve(stringFragments));
		}
	}

	//Read String index table
	SDL_RWseek(fp, sizeof(head), RW_SEEK_SET);
	Vector<locTableEntry> tables(head.count);
	SDL_RWread(fp, tables.data(), sizeof(locTableEntry), tables.size());

	//Read Strings
	for (locTableEntry &entry : tables) {
		SDL_Log("%u  %u\n", entry.idOffset, entry.offset);
		//SDL_assert_release(entry.offset < head.fragmentOffset);

		uint16_t entryCount = SDL_ReadLE16(fp);
		for (int j = 0; j < entryCount; j++) {
			uint32_t id = entry.idOffset + SDL_ReadLE16(fp);
			uint16_t count = SDL_ReadLE16(fp);
		}
	}
	//SDL_assert_release(SDL_RWtell(fp) == head.fragmentOffset);

	SDL_RWclose(fp);
	return true;
}

std::wstring locFragment::resolve(const Vector<locFragment> &list) const {
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
