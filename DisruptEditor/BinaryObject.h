#pragma once

#include <map>
#include <vector>
#include <string>
#include <stdio.h>
#include <memory>

class BinaryObject;

uint32_t ReadCount(FILE *fp, bool &isOffset);
uint32_t ReadCountB(FILE *fp, bool &isOffset);
std::shared_ptr<BinaryObject> Deserialize(BinaryObject *parent, FILE *fp, std::vector<std::shared_ptr<BinaryObject>>& pointers);

class BinaryObject {
public:
	BinaryObject();

	void Deserialize(FILE *fp, uint32_t childCount, std::vector<std::shared_ptr<BinaryObject>> &pointers);

	long position;
	uint32_t nameHash;
	std::map<uint64_t, std::vector<uint8_t>> fields;
	std::vector<std::shared_ptr<BinaryObject>> children;
};

