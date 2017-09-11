#include "wluFile.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "tinyxml2.h"
#include "BinaryObject.h"
#include "Hash.h"

class Attribute {
public:
	Attribute(FILE* fp) {
		fread(&hash, sizeof(hash), 1, fp);
	};
	void deserialize(FILE *fp);

	void serializeXML(tinyxml2::XMLPrinter &printer);

	std::string getHashName();
	std::string getHumanReadable();
	std::string getByteString();
	uint32_t hash;
	std::vector<uint8_t> buffer;
};

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

	return str;
}

class Node {
public:
	Node(FILE* fp) { deserialize(fp); };
	void deserialize(FILE *fp);
	void serializeXML(tinyxml2::XMLPrinter &printer);

	std::string getHashName();

	size_t offset;
	uint32_t hash;

	std::vector<Node> children;
	std::vector<Attribute> attributes;
};

void Node::deserialize(FILE* fp) {
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
		assert(num1 != 0);

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
		for(auto &attribute : attributes)
			attribute.deserialize(fp);

		size_t a = ftell(fp);
		assert(a == num2);

		children.reserve(c);
		for (int index = 0; index < c; ++index)
			children.emplace_back(fp);
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
	if (memcmp(wluhead.base.magic, "ESAB", 4) != 0) {
		fclose(fp);
		return false;
	}

	Node node(fp);
	fclose(fp);

	fp = fopen("test.xml", "wb");
	tinyxml2::XMLPrinter printer(fp);
	node.serializeXML(printer);
	fclose(fp);

	int count = 0;

	/*while (ftell(fp) < wluhead.size + sizeof(wluhead)) {
		//size_t a = ftell(fp);
		//printf("Offset: %d\n", a);

		uint32_t c = ReadCountB(fp);
		if (c < 254)
			printf("%02X\n", c);
		else
			printf("%08X\n", c);

		uint32_t hash;
		fread(&hash, sizeof(hash), 1, fp);
		printf("%08X\n", hash);

		uint8_t s;
		fread(&s, 1, 1, fp);
		printf("%02X\n", s);

		std::vector<uint8_t> data(s + 1);
		fread(data.data(), 1, data.size(), fp);
		for (auto it = data.begin(); it != data.end(); ++it) {
			if(*it > 32 && *it < 126)
				printf("%c", *it);
			else
				printf("%02X ", *it);
		}
		printf("\n\n");
		//printf("Datasize: %d\n", data.size());

		count++;
	}*/

	//std::vector<std::shared_ptr<BinaryObject>> pointers;
	//std::shared_ptr<BinaryObject> root = Deserialize(NULL, fp, pointers);

	//size_t a = ftell(fp);
	//printf("Final Offset: %d\n", a);

	fclose(fp);
	return true;
}
