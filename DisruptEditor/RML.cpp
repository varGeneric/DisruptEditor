#include "RML.h"

#include <SDL_assert.h>
#include <SDL_rwops.h>
#include "Vector.h"

struct RmlHeader {
	uint8_t magic;
	uint8_t unknown;
	uint32_t stringTableSize, totalNodeCount, totalAttributeCount;
};

uint32_t readPackedU32(SDL_RWops* fp) {
	uint8_t value = SDL_ReadU8(fp);
	if (value < 0xFE) {
		return value;
	}

	SDL_assert_release(value != 0xFE);

	return SDL_ReadLE32(fp);
}

struct RmlAttribute {
	uint32_t nameIndex, valueIndex;
	void deserialize(SDL_RWops *fp);
};

void RmlAttribute::deserialize(SDL_RWops * fp) {
	uint32_t unknown = readPackedU32(fp);
	SDL_assert_release(unknown == 0);
	nameIndex = readPackedU32(fp);
	valueIndex = readPackedU32(fp);
}

struct RmlNode {
	uint32_t nameIndex, valueIndex;
	Vector<RmlAttribute> attributes;
	Vector<RmlNode> children;
	void deserialize(SDL_RWops *fp);
	tinyxml2::XMLElement* serializeXML(tinyxml2::XMLDocument* doc, tinyxml2::XMLElement *parent, const Vector<char> &strTable);
};

void RmlNode::deserialize(SDL_RWops * fp) {
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

tinyxml2::XMLElement* RmlNode::serializeXML(tinyxml2::XMLDocument *doc, tinyxml2::XMLElement *parent, const Vector<char> &strTable) {
	tinyxml2::XMLElement *me = doc->NewElement(&strTable[nameIndex]);

	if (parent)
		parent->InsertEndChild(me);

	for (RmlAttribute &attr : attributes)
		me->SetAttribute(&strTable[attr.nameIndex], &strTable[attr.valueIndex]);

	for (RmlNode &child : children)
		child.serializeXML(doc, me, strTable);

	return me;
}

std::unique_ptr<tinyxml2::XMLDocument> loadRml(const char * filename) {
	std::unique_ptr<tinyxml2::XMLDocument> doc = std::make_unique<tinyxml2::XMLDocument>();

	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	if (!fp) return NULL;
	Vector<uint8_t> data(SDL_RWsize(fp));
	SDL_RWread(fp, data.data(), data.size(), 1);
	SDL_RWclose(fp);
	fp = SDL_RWFromConstMem(data.data(), data.size());

	RmlHeader head;
	head.magic = SDL_ReadU8(fp);
	head.unknown = SDL_ReadU8(fp);
	head.stringTableSize = readPackedU32(fp);
	head.totalNodeCount = readPackedU32(fp);
	head.totalAttributeCount = readPackedU32(fp);

	RmlNode root;
	root.deserialize(fp);

	Vector<char> stringTableData(head.stringTableSize);
	SDL_RWread(fp, stringTableData.data(), 1, head.stringTableSize);
	SDL_RWclose(fp);

	doc->InsertFirstChild(root.serializeXML(doc.get(), NULL, stringTableData));

	return doc;
}
