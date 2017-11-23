#include "locFile.h"

#include <algorithm>
#include <SDL.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include "Vector.h"
#include <SDL_stdinc.h>

#pragma pack(push, 1)
struct locHeader {
	uint16_t magic;
	uint16_t one;
	uint16_t langCode;
	uint16_t count;
	uint32_t fragmentOffset;
	void swapEndianess();
};
struct locTableEntry {
	uint32_t idOffset;
	uint32_t offset;
	void swapEndianess();
};
struct locFragment {
	uint16_t rightIndex = 0;
	uint16_t leftIndex = 0;
	std::wstring resolve(const Vector<locFragment> &list) const;
	void swapEndianess();
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

	bool bigEndian = false;

	//Read Header
	locHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	if (head.magic != 19539) {
		head.swapEndianess();
		bigEndian = true;
	}
	SDL_assert_release(head.magic == 19539);
	SDL_assert_release(head.one == 1);
	SDL_assert_release(head.fragmentOffset <= SDL_RWsize(fp));

	//Read String Fragment Table
	SDL_RWseek(fp, head.fragmentOffset, RW_SEEK_SET);
	Vector<std::wstring> strings;
	uint32_t numStringFrags;
	int stringFragmentIndexMask;
	{
		numStringFrags = bigEndian ? SDL_ReadBE32(fp) : SDL_ReadLE32(fp);
		stringFragmentIndexMask = numStringFrags * 255;
		Vector<locFragment> stringFragments(numStringFrags);
		SDL_RWread(fp, stringFragments.data() + 1, sizeof(locFragment), stringFragments.size() - 1);
		SDL_assert_release(SDL_RWtell(fp) == SDL_RWsize(fp));

		if (bigEndian) {
			for (locFragment &frag : stringFragments)
				frag.swapEndianess();
		}

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
	if (bigEndian) {
		for (locTableEntry &entry : tables)
			entry.swapEndianess();
	}

	//Read Strings
	for (locTableEntry &entry : tables) {
		SDL_Log("%u  %u\n", entry.idOffset, entry.offset);
		//SDL_assert_release(entry.offset < head.fragmentOffset);

		/*uint16_t entryCount = SDL_ReadLE16(fp);
		for (int j = 0; j < entryCount; j++) {
			uint32_t id = entry.idOffset + SDL_ReadLE16(fp);
			uint16_t count = SDL_ReadLE16(fp);
		}*/
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

void locFragment::swapEndianess() {
	rightIndex = SDL_Swap16(rightIndex);
	leftIndex = SDL_Swap16(leftIndex);
	std::swap(rightIndex, leftIndex);
}

void locHeader::swapEndianess() {
	magic = SDL_Swap16(magic);
	one = SDL_Swap16(one);
	langCode = SDL_Swap16(langCode);
	count = SDL_Swap16(count);
	fragmentOffset = SDL_Swap32(fragmentOffset);
}

void locTableEntry::swapEndianess() {
	idOffset = SDL_Swap32(idOffset);
	offset = SDL_Swap32(offset);
}
