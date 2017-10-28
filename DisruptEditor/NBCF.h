#pragma once

#include <vector>
#include "tinyxml2.h"

struct fcbHeader {
	char magic[4];
	uint16_t version;
	uint16_t headerFlags;
	uint32_t totalObjectCount;
	uint32_t totalValueCount;
};

class Attribute {
public:
	Attribute() {}
	Attribute(FILE* fp) {
		fread(&hash, sizeof(hash), 1, fp);
	};
	void deserializeA(FILE *fp);
	void deserialize(FILE *fp, bool &bailOut);
	void serialize(FILE *fp);
	void deserializeXML(const tinyxml2::XMLAttribute *attr);
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
	void deserialize(FILE *fp, bool &bailOut);
	void deserializeA(FILE *fp);
	void serialize(FILE *fp);
	void deserializeXML(const tinyxml2::XMLElement *node);
	void serializeXML(tinyxml2::XMLPrinter &printer);

	Node* findFirstChild(const char* name);
	Node* findFirstChild(uint32_t hash);
	Attribute* getAttribute(const char* name);
	Attribute* getAttribute(uint32_t hash);

	int countNodes();

	std::string getHashName();

	size_t offset;
	uint32_t hash;

	std::vector<Node> children;
	std::vector<Attribute> attributes;
};
