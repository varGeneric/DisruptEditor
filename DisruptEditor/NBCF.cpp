#include "NBCF.h"

#include <SDL_assert.h>
#include <SDL_log.h>
#include "Hash.h"

uint32_t ReadCountA(FILE *fp, bool &isOffset) {
	uint8_t value;
	fread(&value, sizeof(value), 1, fp);
	isOffset = false;

	if (value < 0xFE) {
		return value;
	}

	isOffset = value != 0xFF;

	uint32_t v;
	fread(&v, sizeof(v), 1, fp);
	return v;
}

uint32_t ReadCountB(FILE *fp, bool &isOffset) {
	size_t pos = ftell(fp);

	uint8_t value;
	fread(&value, sizeof(value), 1, fp);

	isOffset = false;

	if (value < 0xFE)
		return value;
	else {
		isOffset = value == 0xFE;

		fseek(fp, -1, SEEK_CUR);
		uint32_t v;
		fread(&v, sizeof(v), 1, fp);
		v = v >> 8;

		if (isOffset) {
			v = pos - v;
		}

		return v;
	}
}

void writeSize(FILE *fp, size_t osize) {
	SDL_assert_release(osize != 254);
	if (osize > 254) {
		uint32_t size = osize;
		size = size << 8;
		size |= 0xFF;
		fwrite(&size, sizeof(size), 1, fp);
	} else {
		uint8_t size = osize;
		fwrite(&size, sizeof(size), 1, fp);
	}
}

void Attribute::deserializeA(FILE * fp) {
	fread(&hash, sizeof(hash), 1, fp);

	bool isOffset;
	uint32_t size = ReadCountA(fp, isOffset);

	size_t position = ftell(fp);

	if (isOffset) {
		fseek(fp, position - size, SEEK_SET);

		size = ReadCountA(fp, isOffset);
		SDL_assert_release(!isOffset);

		buffer.resize(size);
		fread(buffer.data(), 1, size, fp);

		fseek(fp, position, SEEK_SET);
	} else {
		buffer.resize(size);
		fread(buffer.data(), 1, size, fp);
	}
}

void Attribute::deserialize(FILE * fp, bool &bailOut) {
	size_t offset = ftell(fp);

	bool isOffset;
	int32_t c = ReadCountB(fp, isOffset);
	if (isOffset) {
		fseek(fp, c, SEEK_SET);
		deserialize(fp, bailOut);
		fseek(fp, offset + 4, SEEK_SET);
	} else {
		if (c > 1024 * 100) {//Hard limit of 100 kb
			bailOut = true;
			return;
		}
		buffer.resize(c);
		fread(buffer.data(), 1, c, fp);
	}
}

void Attribute::serialize(FILE * fp) {
	writeSize(fp, buffer.size());
	fwrite(buffer.data(), 1, buffer.size(), fp);
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

void Node::deserialize(FILE* fp, bool &bailOut) {
	if (bailOut) return;
	size_t pos = ftell(fp);

	bool isOffset;
	int32_t c = ReadCountB(fp, isOffset);

	if (isOffset) {
		fseek(fp, c, SEEK_SET);
		deserialize(fp, bailOut);
		fseek(fp, pos + 4, SEEK_SET);
	} else {
		offset = pos;
		fread(&hash, sizeof(hash), 1, fp);

		uint16_t num1;
		fread(&num1, sizeof(num1), 1, fp);

		size_t pos2 = ftell(fp);
		size_t num2 = pos2 + num1;
		//SDL_assert_release(num1);

		bool isOffset2;
		int32_t c2 = ReadCountB(fp, isOffset2);
		bool flag = false;
		if (isOffset2) {
			fseek(fp, c2, SEEK_SET);

			bool isOffset3;
			c2 = ReadCountB(fp, isOffset3);
			SDL_assert_release(!isOffset3);
			pos2 += 4;
			flag = true;
		}

		attributes.resize(c2);
		for (int index = 0; index < c2; ++index) {
			attributes[index] = Attribute(fp);
		}
		if (flag)
			fseek(fp, pos2, SEEK_SET);
		for (auto &attribute : attributes) {
			attribute.deserialize(fp, bailOut);
			if (bailOut)
				return;
		}

		size_t a = ftell(fp);
		if (a != num2) {
			SDL_Log("Warning! This file could not be read!\n");
			//bailOut = true;
			//return;
			//fseek(fp, num2, SEEK_SET);
		}

		children.resize(c);
		for (int index = 0; index < c; ++index)
			children[index].deserialize(fp, bailOut);
	}
}

void Node::deserializeA(FILE * fp, Vector<Node*> &list) {
	bool isOffset;
	uint32_t childCount = ReadCountA(fp, isOffset);

	if (isOffset) {
		SDL_assert_release(list.size() > childCount);
		*this = *list[childCount];
		return;
	} else {
		fread(&hash, sizeof(hash), 1, fp);

		uint32_t attributeCount = ReadCountA(fp, isOffset);
		SDL_assert_release(!isOffset);

		attributes.resize(attributeCount);
		for (uint32_t i = 0; i < attributeCount; ++i) {
			attributes[i].deserializeA(fp);
		}
	}

	list.push_back(this);

	children.resize(childCount);
	for (int index = 0; index < childCount; ++index)
		children[index].deserializeA(fp, list);
}

void Node::serialize(FILE * fp) {
	writeSize(fp, children.size());
	fwrite(&hash, sizeof(hash), 1, fp);

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
	fwrite(&num1, sizeof(num1), 1, fp);

	//Write Attribute Hashes
	writeSize(fp, attributes.size());
	for (auto &attribute : attributes) {
		fwrite(&attribute.hash, sizeof(attribute.hash), 1, fp);
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

	FILE *fp = fopen(filename, "rb");
	if (!fp)
		return node;

	fcbHeader head;
	fread(&head, sizeof(head), 1, fp);

	if (memcmp(head.magic, "nbCF", 4) != 0) {
		fclose(fp);
		return node;
	}

	if (head.version == 16389) {
		bool bailOut = false;
		node.deserialize(fp, bailOut);
	} else if (head.version == 3) {
		Vector<Node*> list;
		node.deserializeA(fp, list);
	}

	return node;
}
