#include "BinaryObject.h"



BinaryObject::BinaryObject() {
}

uint32_t ReadCount(FILE *fp, bool &isOffset) {
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

std::shared_ptr<BinaryObject> Deserialize(BinaryObject *parent, FILE *fp, std::vector<std::shared_ptr<BinaryObject>>& pointers) {
	long position = ftell(fp);

	bool isOffset;
	uint32_t childCount = ReadCount(fp, isOffset);

	if (isOffset) {
		return pointers[(int)childCount];
	}

	std::shared_ptr<BinaryObject> child = std::make_shared<BinaryObject>();
	child->position = position;
	pointers.push_back(child);

	child->Deserialize(fp, childCount, pointers);
	return child;
}

void BinaryObject::Deserialize(FILE * fp, uint32_t childCount, std::vector<std::shared_ptr<BinaryObject>>& pointers) {
	bool isOffset;

	fread(&nameHash, sizeof(nameHash), 1, fp);

	uint32_t valueCount = ReadCount(fp, isOffset);
	if (isOffset) {
		abort();
	}

	fields.clear();
	for (uint32_t i = 0; i < valueCount; ++i) {
		uint32_t nameHash;
		fread(&nameHash, sizeof(nameHash), 1, fp);
		std::vector<uint8_t> value;

		long position = ftell(fp);
		uint32_t size = ReadCount(fp, isOffset);
		if (isOffset) {
			fseek(fp, position - size, SEEK_SET);

			size = ReadCount(fp, isOffset);
			if (isOffset) {
				abort();
			}

			value.resize(size);
			fread(value.data(), 1, size, fp);

			fseek(fp, position, SEEK_SET);
			ReadCount(fp, isOffset);
		}
		else {
			value.resize(size);
			fread(value.data(), 1, size, fp);
		}

		fields[nameHash] = value;
	}

	children.clear();
	for (uint32_t i = 0; i < childCount; i++) {
		children.push_back(::Deserialize(this, fp, pointers));
	}
}
