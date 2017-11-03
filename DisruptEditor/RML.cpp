#include "RML.h"

#include <SDL_assert.h>
#include "Vector.h"

struct RmlHeader {
	uint8_t magic;
	uint8_t unknown;
	uint32_t stringTableSize, totalNodeCount, totalAttributeCount;
};

uint32_t readPackedU32(FILE* fp) {
	uint8_t value;
	fread(&value, sizeof(value), 1, fp);
	if (value < 0xFE) {
		return value;
	}

	SDL_assert_release(value != 0xFE);

	uint32_t fullValue;
	fread(&fullValue, sizeof(fullValue), 1, fp);
	return fullValue;
}

struct RmlAttribute {
	uint32_t nameIndex, valueIndex;
	void deserialize(FILE *fp);
};

void RmlAttribute::deserialize(FILE * fp) {
	uint32_t unknown = readPackedU32(fp);
	SDL_assert_release(unknown == 0);
	nameIndex = readPackedU32(fp);
	valueIndex = readPackedU32(fp);
}

struct RmlNode {
	uint32_t nameIndex, valueIndex;
	Vector<RmlAttribute> attributes;
	Vector<RmlNode> children;
	void deserialize(FILE *fp);
};

void RmlNode::deserialize(FILE * fp) {
	nameIndex = readPackedU32(fp);
	valueIndex = readPackedU32(fp);

	uint32_t attributeCount = readPackedU32(fp);
	uint32_t childCount = readPackedU32(fp);

	attributes.resize(attributeCount);
	for (uint32_t i = 0; i < attributeCount; ++i) {
		attributes[i].deserialize(fp);
	}

	children.resize(childCount);
	for (uint32_t i = 0; i < childCount; ++i) {
		children[i].deserialize(fp);
	}
}

std::unique_ptr<tinyxml2::XMLDocument> loadRml(const char * filename) {
	std::unique_ptr<tinyxml2::XMLDocument> doc = std::make_unique<tinyxml2::XMLDocument>();

	FILE *fp = fopen(filename, "rb");
	if (!fp) return nullptr;

	RmlHeader head;
	fread(&head.magic, sizeof(head.magic), 1, fp);
	fread(&head.unknown, sizeof(head.unknown), 1, fp);
	head.stringTableSize = readPackedU32(fp);
	head.totalNodeCount = readPackedU32(fp);
	head.totalAttributeCount = readPackedU32(fp);

	RmlNode root;
	root.deserialize(fp);

	Vector<char> stringTableData(head.stringTableSize);
	fread(stringTableData.data(), 1, head.stringTableSize, fp);
	fclose(fp);



	return doc;
}
