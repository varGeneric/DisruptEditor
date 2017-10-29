#pragma once

#include "Vector.h"
#include <string>
#include <map>

class DominoFunction {
public:
	DominoFunction() {}
	std::string deserialize(FILE *fp);

	std::string str;
};

class DominoCBox {
public:
	DominoCBox() {}
	std::string deserialize(FILE *fp);

	int id;
	std::string boxClass;
	std::map<std::string, std::string> localVariables;
};

class DominoBox {
public:
	DominoBox(const char* filename);

	Vector<std::string> CBoxDeps;
	std::map<int, DominoCBox> boxes;
	std::map<std::string, DominoFunction> functions;
	std::map<std::string, std::string> localVariables;
};

