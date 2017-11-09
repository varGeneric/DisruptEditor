#pragma once

#include "Vector.h"
#include <string>
#include "tinyxml2.h"
#include <SDL_rwops.h>

#pragma pack(push, 1)
struct fcbHeader {
	char magic[4];
	uint16_t version;
	uint16_t headerFlags;
	uint32_t totalObjectCount;
	uint32_t totalValueCount;
};
#pragma pack(pop)

class Attribute {
public:
	Attribute() {}
	Attribute(SDL_RWops* fp) {
		hash = SDL_ReadLE32(fp);
	};
	void deserializeA(SDL_RWops *fp);
	void deserialize(SDL_RWops *fp, bool &bailOut);
	void serialize(SDL_RWops *fp);
	void deserializeXML(const tinyxml2::XMLAttribute *attr);
	void serializeXML(tinyxml2::XMLPrinter &printer);

	std::string getHashName();
	std::string getHumanReadable();
	std::string getByteString();
	uint32_t hash;
	Vector<uint8_t> buffer;
};

class Node {
public:
	Node() {};
	void deserialize(SDL_RWops *fp, bool &bailOut);
	void deserializeA(SDL_RWops *fp, Vector<Node*> &list);
	void serialize(SDL_RWops *fp);
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

	Vector<Node> children;
	Vector<Attribute> attributes;
};

Node readFCB(const char* filename);
