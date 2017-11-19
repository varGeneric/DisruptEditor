#pragma once

#include "Vector.h"
#include <string>
#include <map>
#include "glm/glm.hpp"
#include "tinyxml2.h"

enum DominoSlotType { DATA, CONTROL };

class DominoPtr {
public:
	int from, to;
	DominoSlotType type;
	std::string fromKey, toKey;
};

class DominoSlot {
public:
	std::string name, value;
	DominoSlotType type;
};

class DominoCBox {
public:
	DominoCBox() {}
	std::string deserialize(FILE *fp);

	glm::vec2 pos, size;

	int id;
	std::string boxClass;
	std::string getShortName();
	Vector<DominoSlot> in, out;

	glm::vec2 GetInputSlotPos(int slot_no) const { return glm::vec2(pos.x, pos.y - 10.f + size.y * ((float)slot_no + 1) / ((float)in.size() + 1)); }
	glm::vec2 GetOutputSlotPos(int slot_no) const { return glm::vec2(pos.x + size.x, pos.y - 10.f + size.y * ((float)slot_no + 1) / ((float)out.size() + 1)); }
};

class DominoBox {
public:
	void open(const char* filename);

	void serialize(tinyxml2::XMLPrinter &printer);

	void draw();
	void autoOrganize();

	Vector<DominoPtr> connections;
	std::map<int, DominoCBox> boxes;
	std::map<std::string, std::string> localVariables;

private:
	glm::vec2 scrolling;
};

