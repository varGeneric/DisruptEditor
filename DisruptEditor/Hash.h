#pragma once

#include <map>
#include <string>

class Hash {
public:
	Hash();
	static Hash& instance();
	std::string getReverseHash(uint32_t hash);
private:
	std::map<uint32_t, std::string> reverseHash;
};

