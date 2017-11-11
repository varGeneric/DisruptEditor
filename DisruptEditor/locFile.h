#pragma once

#include <unordered_map>
#include <string>

class locFile {
public:
	bool open(const char* filename);
	std::unordered_map<uint32_t, std::string> uncompiledStrings;
};

