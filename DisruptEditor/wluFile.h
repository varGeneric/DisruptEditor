#pragma once

#include <stdint.h>
#include "tinyxml2.h"
#include <string>
#include <vector>

struct baseHeader {
	char magic[4];
	uint32_t size;
	uint32_t unknown1;
	uint32_t unknown2;
};

struct fcbHeader {
	char magic[4];
	uint16_t version;
	uint16_t headerFlags;
	uint32_t totalObjectCount;
	uint32_t totalValueCount;
};

struct wluHeader {
	baseHeader base;
	fcbHeader fcb;
};

class Attribute {
public:
	Attribute(FILE* fp) {
		fread(&hash, sizeof(hash), 1, fp);
	};
	void deserialize(FILE *fp);
	void serialize(FILE *fp);

	void serializeXML(tinyxml2::XMLPrinter &printer);

	std::string getHashName();
	std::string getHumanReadable();
	std::string getByteString();
	uint32_t hash;
	std::vector<uint8_t> buffer;
};

class Node {
public:
	Node() {};
	Node(FILE* fp) { deserialize(fp); };
	void deserialize(FILE *fp);
	void serialize(FILE *fp);
	void serializeXML(tinyxml2::XMLPrinter &printer);

	Node* findFirstChild(const char* name);
	Attribute* getAttribute(const char* name);
	Attribute* getAttribute(uint32_t hash);

	std::string getHashName();

	size_t offset;
	uint32_t hash;

	std::vector<Node> children;
	std::vector<Attribute> attributes;
};

class wluFile {
public:
	wluFile() {};
	bool open(const char* filename);

	void serialize(FILE *fp);

	Node root;
};

