#pragma once

#include <map>
#include <string>

class Hash {
public:
	Hash();
	static uint32_t crcHash(void* data, size_t size);
	static Hash& instance();
	std::string getReverseHash(uint32_t hash);
	uint32_t getHash(const char* str);
private:
	std::map<uint32_t, std::string> reverseHash;
};

