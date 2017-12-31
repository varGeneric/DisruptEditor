#include "NBCF.h"

#include <SDL_assert.h>
#include <SDL_log.h>
#include <SDL_endian.h>
#include "Hash.h"

uint32_t ReadCountA(SDL_RWops *fp, bool &isOffset, bool bigEndian) {
	uint8_t value = SDL_ReadU8(fp);
	isOffset = false;

	if (value < 0xFE) {
		return value;
	}

	isOffset = value != 0xFF;
	return bigEndian ? SDL_ReadBE32(fp) : SDL_ReadLE32(fp);
}

uint32_t ReadCountB(SDL_RWops *fp, bool &isOffset, bool bigEndian) {
	size_t pos = SDL_RWtell(fp);

	uint8_t value = SDL_ReadU8(fp);

	isOffset = false;

	if (value < 0xFE)
		return value;
	else {
		isOffset = value == 0xFE;

		SDL_RWseek(fp, -1, RW_SEEK_CUR);
		uint32_t v = bigEndian ? SDL_ReadBE32(fp) : SDL_ReadLE32(fp);
		v = v >> 8;

		if (isOffset) {
			v = pos - v;
		}

		return v;
	}
}

void writeSize(SDL_RWops *fp, size_t osize) {
	SDL_assert_release(osize != 254);
	if (osize > 254) {
		uint32_t size = osize;
		size = size << 8;
		size |= 0xFF;
		SDL_WriteLE32(fp, size);
	} else {
		SDL_WriteU8(fp, (uint8_t)osize);
	}
}

Attribute::Attribute(SDL_RWops * fp, bool bigEndian) {
	hash = bigEndian ? SDL_ReadBE32(fp) : SDL_ReadLE32(fp);
}

void Attribute::deserializeA(SDL_RWops * fp, bool bigEndian) {
	hash = bigEndian ? SDL_ReadBE32(fp) : SDL_ReadLE32(fp);

	bool isOffset;
	size_t position = SDL_RWtell(fp);
	uint32_t size = ReadCountA(fp, isOffset, bigEndian);

	if (isOffset) {
		SDL_RWseek(fp, position - size, RW_SEEK_SET);

		size = ReadCountA(fp, isOffset, bigEndian);
		SDL_assert_release(!isOffset);

		buffer.resize(size);
		SDL_RWread(fp, buffer.data(), 1, size);

		SDL_RWseek(fp, position, RW_SEEK_SET);
		ReadCountA(fp, isOffset, bigEndian);
	} else {
		buffer.resize(size);
		SDL_RWread(fp, buffer.data(), 1, size);
	}
}

void Attribute::deserialize(SDL_RWops * fp, bool bigEndian) {
	size_t offset = SDL_RWtell(fp);

	bool isOffset;
	int32_t c = ReadCountB(fp, isOffset, bigEndian);
	if (isOffset) {
		SDL_RWseek(fp, c, RW_SEEK_SET);
		deserialize(fp, bigEndian);
		SDL_RWseek(fp, offset + 4, RW_SEEK_SET);
	} else {
		SDL_assert_release(c < 1024 * 400);//Hard limit of 400 kb
		buffer.resize(c);
		SDL_RWread(fp, buffer.data(), 1, c);
	}
}

void Attribute::serialize(SDL_RWops * fp) {
	writeSize(fp, buffer.size());
	SDL_RWwrite(fp, buffer.data(), 1, buffer.size());
}

void Attribute::deserializeXML(const tinyxml2::XMLAttribute *attr) {
	const char* name = attr->Name();
	//Determine if this is a hash or name
	if (name[0] == '_') {
		name++;
		hash = std::stoul(name, NULL, 16);
	} else {
		hash = Hash::instance().getHash(name);
	}

	//Determine if this is hex string or string
	const char* data = attr->Value();

	bool hex = true;
	for (;; data++) {
		char c = *data;
		if (c == '\0')
			break;
		bool valid = (c >= 48 && c <= 57) || (c >= 97 && c <= 102) || c == ' ';
		if (!valid) {
			hex = false;
			break;
		}
	}

	data = attr->Value();

	if (hex) {
		buffer.clear();
		if (strlen(data) == 0) return;
		int len = (strlen(data) / 3) + 1;
		buffer.reserve(len);

		for (int i = 0; i < len; ++i) {
			uint8_t byte = (uint8_t)strtol(data, NULL, 16);
			buffer.push_back(byte);
			data += 3;
		}
	} else {
		buffer.resize(strlen(data) + 1);
		strcpy((char*)buffer.data(), data);
	}

}

void Attribute::serializeXML(tinyxml2::XMLPrinter &printer) {
	printer.PushAttribute(getHashName().c_str(), getHumanReadable().c_str());
}

std::string Attribute::getHashName() {
	return Hash::instance().getReverseHash(hash);
}

std::string Attribute::getHumanReadable() {
	if (buffer.size() > 1 && *buffer.back() == '\0') {
		//Iterate over string for any non-ascii chars
		bool valid = true;
		for (auto byte = buffer.begin(); byte != buffer.end() - 1; ++byte) {
			if (!(*byte > 31 && *byte < 127)) {
				valid = false;
				break;
			}
		}

		if (valid)
			return std::string((char*)buffer.data());
	}

	return getByteString();
}

std::string Attribute::getByteString() {
	std::string str;

	for (auto byte : buffer) {
		char strbuffer[4];
		snprintf(strbuffer, sizeof(strbuffer), "%02x ", byte);
		str.append(strbuffer);
	}

	if (!str.empty())
		str.pop_back();

	return str;
}

void Node::deserialize(SDL_RWops* fp, bool bigEndian) {
	size_t pos = SDL_RWtell(fp);

	bool isOffset;
	int32_t c = ReadCountB(fp, isOffset, bigEndian);

	if (isOffset) {
		SDL_RWseek(fp, c, RW_SEEK_SET);
		deserialize(fp, bigEndian);
		SDL_RWseek(fp, pos + 4, RW_SEEK_SET);
	} else {
		offset = pos;
		hash = bigEndian ? SDL_ReadBE32(fp) : SDL_ReadLE32(fp);

		uint16_t num1 = bigEndian ? SDL_ReadBE16(fp) : SDL_ReadLE16(fp);

		size_t pos2 = SDL_RWtell(fp);
		size_t num2 = pos2 + num1;
		//SDL_assert_release(num1);

		bool isOffset2;
		int32_t c2 = ReadCountB(fp, isOffset2, bigEndian);
		bool flag = false;
		if (isOffset2) {
			SDL_RWseek(fp, c2, RW_SEEK_SET);

			bool isOffset3;
			c2 = ReadCountB(fp, isOffset3, bigEndian);
			SDL_assert_release(!isOffset3);
			pos2 += 4;
			flag = true;
		}

		attributes.resize(c2);
		for (int index = 0; index < c2; ++index) {
			attributes[index] = Attribute(fp, bigEndian);
		}
		if (flag)
			SDL_RWseek(fp, pos2, RW_SEEK_SET);
		for (auto &attribute : attributes) {
			attribute.deserialize(fp, bigEndian);
		}

		size_t a = SDL_RWtell(fp);
		if (a != num2) {
			SDL_Log("Warning! This file could not be read!\n");
			//bailOut = true;
			//return;
			//fseek(fp, num2, SEEK_SET);
		}

		children.resize(c);
		for (int index = 0; index < c; ++index)
			children[index].deserialize(fp, bigEndian);
	}
}

void Node::deserializeA(SDL_RWops * fp, Vector<Node*> &list, bool bigEndian) {
	bool isOffset;
	uint32_t childCount = ReadCountA(fp, isOffset, bigEndian);
	offset = SDL_RWtell(fp);

	if (isOffset) {
		SDL_assert_release(list.size() > childCount);
		*this = *list[childCount];
		return;
	} else {
		hash = bigEndian ? SDL_ReadBE32(fp) : SDL_ReadLE32(fp);

		uint32_t attributeCount = ReadCountA(fp, isOffset, bigEndian);
		SDL_assert_release(!isOffset);

		attributes.resize(attributeCount);
		for (uint32_t i = 0; i < attributeCount; ++i) {
			attributes[i].deserializeA(fp, bigEndian);
		}
	}

	list.push_back(this);

	children.resize(childCount);
	for (int index = 0; index < childCount; ++index)
		children[index].deserializeA(fp, list, bigEndian);
}

void Node::serialize(SDL_RWops * fp) {
	writeSize(fp, children.size());
	SDL_WriteLE32(fp, hash);

	//Calc Attribute Size
	uint16_t num1 = 0;
	if (attributes.size() > 254)
		num1 += 4;
	else
		num1 += 1;
	for (auto &attribute : attributes) {
		num1 += sizeof(attribute.hash);
		num1 += attribute.buffer.size();
		if (attribute.buffer.size() > 254)
			num1 += 4;
		else
			num1 += 1;
	}
	SDL_WriteLE16(fp, num1);

	//Write Attribute Hashes
	writeSize(fp, attributes.size());
	for (auto &attribute : attributes) {
		SDL_WriteLE32(fp, attribute.hash);
	}
	for (auto &attribute : attributes) {
		attribute.serialize(fp);
	}

	//Write Children
	for (auto &child : children) {
		child.serialize(fp);
	}
}

void Node::deserializeXML(const tinyxml2::XMLElement *node) {
	const char* name = node->Name();
	//Determine if this is a hash or name
	if (name[0] == '_') {
		name++;
		hash = std::stoul(name, NULL, 16);
	} else {
		hash = Hash::instance().getHash(name);
	}

	//Load Attributes
	attributes.clear();
	for (auto it = node->FirstAttribute(); it; it = it->Next()) {
		attributes.push_back().deserializeXML(it);
	}

	//Load Children
	children.clear();
	for (auto it = node->FirstChildElement(); it; it = it->NextSiblingElement()) {
		children.push_back().deserializeXML(it);
	}
}

void Node::serializeXML(tinyxml2::XMLPrinter &printer) {
	std::string hashName = getHashName();

	//printer.PushComment((std::string("Offset: ") + std::to_string(offset)).c_str());
	printer.OpenElement(hashName.c_str());

	for (auto &attribute : attributes)
		attribute.serializeXML(printer);

	for (auto &child : children)
		child.serializeXML(printer);

	printer.CloseElement();
}

Node* Node::findFirstChild(const char *name) {
	uint32_t hash = Hash::instance().getHash(name);

	for (auto &child : children) {
		if (child.hash == hash)
			return &child;
	}

	return NULL;
}

Node* Node::findFirstChild(uint32_t hash) {
	for (auto &child : children) {
		if (child.hash == hash)
			return &child;
	}

	return NULL;
}

Attribute* Node::getAttribute(const char *name) {
	uint32_t hash = Hash::instance().getHash(name);

	for (auto &attribute : attributes) {
		if (attribute.hash == hash)
			return &attribute;
	}

	return NULL;
}

Attribute* Node::getAttribute(uint32_t hash) {
	for (auto &attribute : attributes) {
		if (attribute.hash == hash)
			return &attribute;
	}

	return NULL;
}

int Node::countNodes() {
	int num = children.size();
	for (auto &child : children) {
		num += child.countNodes();
	}
	return num;
}

std::string Node::getHashName() {
	return Hash::instance().getReverseHash(hash);
}

Node readFCB(const char * filename) {
	Node node;

	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	if (!fp)
		return node;

	node = readFCB(fp);

	SDL_RWclose(fp);

	return node;
}

Node readFCB(SDL_RWops * fp) {
	Node node;

	fcbHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	bool bigEndian = false;

	if (memcmp(head.magic, "nbCF", 4) != 0) {
		if (memcmp(head.magic, "FCbn", 4) == 0) {
			bigEndian = true;
			head.swapEndian();
		} else {
			return node;
		}
	}

	if (head.version == 16389) {
		bool bailOut = false;
		node.deserialize(fp, bigEndian);
	} else if (head.version == 3) {
		Vector<Node*> list;
		node.deserializeA(fp, list, bigEndian);
	}

	if (SDL_RWsize(fp) != SDL_RWtell(fp))
		SDL_Log("Warning: Extra data at the end of fcb");

	return node;
}

void writeFCBB(SDL_RWops *fp, Node &node) {
	fcbHeader fcb;
	memcpy(fcb.magic, "nbCF", 4);
	fcb.version = 16389;
	fcb.totalObjectCount = 1 + node.countNodes();
	fcb.totalValueCount = fcb.totalObjectCount - 1;

	SDL_RWwrite(fp, &fcb, sizeof(fcb), 1);
	node.serialize(fp);
}

void fcbHeader::swapEndian() {
	version = SDL_Swap16(version);
	headerFlags = SDL_Swap16(headerFlags);
	totalObjectCount = SDL_Swap32(totalObjectCount);
	totalValueCount = SDL_Swap32(totalValueCount);
}
