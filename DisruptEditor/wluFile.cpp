#include "wluFile.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "tinyxml2.h"
#include "Hash.h"

bool bailOut = false;

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
	assert(osize != 254);
	if (osize > 254) {
		uint32_t size = osize;
		size = size << 8;
		size |= 0xFF;
		//TODO, Check if this works
		fwrite(&size, sizeof(size), 1, fp);
	} else {
		uint8_t size = osize;
		fwrite(&size, sizeof(size), 1, fp);
	}
}

void Attribute::deserialize(FILE * fp) {
	size_t offset = ftell(fp);

	bool isOffset;
	int32_t c = ReadCountB(fp, isOffset);
	if (isOffset) {
		fseek(fp, c, SEEK_SET);
		deserialize(fp);
		fseek(fp, offset + 4, SEEK_SET);
	} else {
		buffer.resize(c);
		fread(buffer.data(), 1, c, fp);
		/*if (this.Type == DataType.BinHex)
		return;
		int attributeTypeSize = Utils.GetAttributeTypeSize(this.Type);
		if (attributeTypeSize == -1)
		return;
		if (this.Type == DataType.StringHash) {
		if ((count > 1?1:(count != 1?0:((int)this.Buffer[0] == 0?1:0))) == 0 || count == 4)
		return;
		this.Type = DataType.String;
		} else if (count > attributeTypeSize)
		throw new InvalidOperationException(string.Format("Data type '{0}' buffer has overflowed (size: 0x{1:X})", (object)this.Type.ToString(), (object)count));*/
	}
}

void Attribute::serialize(FILE * fp) {
	writeSize(fp, buffer.size());
	fwrite(buffer.data(), 1, buffer.size(), fp);
}

void Attribute::serializeXML(tinyxml2::XMLPrinter &printer) {
	printer.PushAttribute(getHashName().c_str(), getHumanReadable().c_str());
}

std::string Attribute::getHashName() {
	return Hash::instance().getReverseHash(hash);
}

std::string Attribute::getHumanReadable() {
	if (buffer.size() > 1 && buffer.back() == '\0') {
		//Iterate over string for any non-ascii chars
		bool valid = true;
		for (auto byte = buffer.begin(); byte != buffer.end() - 1; ++byte) {
			if (!(*byte > 31 && *byte < 127)) {
				valid = false;
				break;
			}
		}

		if(valid)
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

	if(!str.empty())
		str.pop_back();

	return str;
}

void Node::deserialize(FILE* fp) {
	if (bailOut) return;
	size_t pos = ftell(fp);

	bool isOffset;
	int32_t c = ReadCountB(fp, isOffset);

	if (isOffset) {
		fseek(fp, c, SEEK_SET);
		deserialize(fp);
		fseek(fp, pos + 4, SEEK_SET);
	} else {
		offset = pos;
		fread(&hash, sizeof(hash), 1, fp);

		uint16_t num1;
		fread(&num1, sizeof(num1), 1, fp);

		size_t pos2 = ftell(fp);
		size_t num2 = pos2 + num1;
		if (num1 != 0) {
			bool isOffset2;
			int32_t c2 = ReadCountB(fp, isOffset2);
			bool flag = false;
			if (isOffset2) {
				fseek(fp, c2, SEEK_SET);

				bool isOffset3;
				c2 = ReadCountB(fp, isOffset3);
				assert(!isOffset3);
				pos2 += 4;
				flag = true;
			}

			attributes.reserve(c2);
			for (int index = 0; index < c2; ++index)
				attributes.emplace_back(fp);
			if (flag)
				fseek(fp, pos2, SEEK_SET);
			for (auto &attribute : attributes)
				attribute.deserialize(fp);

			size_t a = ftell(fp);
			if (a != num2) {
				printf("Warning! This file could not be read!\n");
				bailOut = true;
				return;
			}
		}

		children.reserve(c);
		for (int index = 0; index < c; ++index)
			children.emplace_back(fp);
	}
}

void Node::serialize(FILE * fp) {
	writeSize(fp, children.size());
	fwrite(&hash, sizeof(hash), 1, fp);

	//Calc Attribute Size
	uint16_t num1 = 0;
	if (!attributes.empty()) {
		if (attributes.size() > 254)
			num1 += 4;
		else
			num1 += 1;
	}
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
	if (!attributes.empty()) {
		writeSize(fp, attributes.size());
	}
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

std::string Node::getHashName() {
	return Hash::instance().getReverseHash(hash);
}

bool wluFile::open(const char * filename) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		return false;
	}

	wluHeader wluhead;
	fread(&wluhead, sizeof(wluhead), 1, fp);

	assert(memcmp(wluhead.base.magic, "ESAB", 4) == 0);
	assert(memcmp(wluhead.fcb.magic, "nbCF", 4) == 0);
	assert(wluhead.fcb.version == 16389);
	assert(wluhead.fcb.totalObjectCount == wluhead.fcb.totalValueCount + 1);

	bailOut = false;
	root = Node(fp);
	fclose(fp);

	return !bailOut;
}

void wluFile::serialize(FILE *fp) {
	wluHeader wluhead;

	memcpy(wluhead.base.magic, "ESAB", 4);
	wluhead.base.unknown1 = wluhead.base.unknown2 = 0;

	memcpy(wluhead.fcb.magic, "nbCF", 4);
	wluhead.fcb.version = 16389;

	fwrite(&wluhead, sizeof(wluhead), 1, fp);

	root.serialize(fp);

	wluhead.base.size = ftell(fp) - sizeof(wluhead.base);
	fseek(fp, 0, SEEK_SET);
	fwrite(&wluhead, sizeof(wluhead), 1, fp);
}
