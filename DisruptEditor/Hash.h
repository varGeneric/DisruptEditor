#pragma once

#include <unordered_map>
#include <string>

class Hash {
public:
	Hash();
	static uint32_t crcHash(void* data, size_t size);
	static Hash& instance();
	std::string getReverseHash(uint32_t hash);
	uint32_t getHash(const char* str);

	//FNV Stuff
	uint32_t getFilenameHash(std::string str);
	std::string getReverseHashFNV(uint32_t hash);

	enum Types { STRING, STRINGHASH, BINHEX, BOOL, FLOAT, INT16, INT32, UINT8, UINT16, UINT32, UINT64, VEC2, VEC3, VEC4 };
	Types getHashType(const char* str);
	Types getHashType(uint32_t hash);
//private:
	void handleFile(const char* file);
	void handleFileFNV(const char * file);
	void handleTypes(const char* file);
	std::unordered_map<uint32_t, std::string> reverseHash;
	std::unordered_map<uint32_t, Types> hashTypes;

	std::unordered_map<uint32_t, std::string> reverseFNVHash;
};

