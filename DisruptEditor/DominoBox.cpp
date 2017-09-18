#include "DominoBox.h"

#include <assert.h>
#include <stdio.h>
#include <string>
#include <regex>

std::string readLuaLine(FILE *fp) {
	char buffer[500];
	fgets(buffer, sizeof(buffer), fp);

	//Cut off \n
	buffer[strlen(buffer) - 1] = '\0';

	return buffer;
}

std::string scanToNextLine(FILE *fp, const char* str) {
	std::string line;
	do {
		line = readLuaLine(fp);
	} while (line != str);
	return readLuaLine(fp);
}

DominoBox::DominoBox(const char *filename) {
	FILE *fp = fopen(filename, "r");

	//Read Dependencies, TODO: Resources
	std::string line = scanToNextLine(fp, "function export:Create(cbox)");
	do {
		std::smatch cm;
		std::regex_match(line, cm, std::regex("(    cbox:RegisterBox\\(\")(.*)(\"\\);)"));

		if(cm.size() == 4)
			CBoxDeps.push_back(cm[2]);

		line = readLuaLine(fp);
	} while (line != "end;");

	//Read Init and local vars
	line = scanToNextLine(fp, "function export:Init(cbox)");

	//I expect this line to be local l0
	assert(line == "    local l0;");
	line = readLuaLine(fp);

	//Local Variables
	do {
		std::smatch cm;
		std::regex_match(line, cm, std::regex("(    self\\.)(.*)( = )(.*)(;)"));

		if (cm.size() == 6)
			localVariables[cm[2]] = cm[4];
		else
			break;

		line = readLuaLine(fp);
	} while (line != "end;");

	//CBoxes
	do {
		std::smatch cm;
		std::regex_match(line, cm, std::regex("    self\\[(.*)\\] = cbox:CreateBox\\(\"(.*)\"\\);"));

		if (cm.size() != 3)
			break;

		DominoCBox cbox;
		cbox.id = std::stoi(cm[1]);
		if (cbox.id == 34)
			int a = 1;
		cbox.boxClass = cm[2];
		line = cbox.deserialize(fp);

		boxes[cbox.id] = cbox;
	} while (line != "end;");

	//Functions
	line = readLuaLine(fp);
	line = readLuaLine(fp);
	do {
		std::smatch cm;
		std::regex_match(line, cm, std::regex("function export:(.*)\\(\\)"));

		if (cm.size() != 2) {
			line = readLuaLine(fp);
			continue;
		}

		std::string name = cm[1];
		DominoFunction func;
		line = func.deserialize(fp);
		functions[name] = func;
	} while (line != "_compilerVersion = 4;");
	
	fclose(fp);
}

std::string DominoCBox::deserialize(FILE *fp) {
	//Skip First Line it's           l0 = self[15];
	std::string line = readLuaLine(fp);
	line = readLuaLine(fp);//l0._graph = self;
	assert(line == "    l0._graph = self;");
	line = readLuaLine(fp);

	do {
		std::smatch cm;
		std::regex_match(line, cm, std::regex("    self\\[(.*)\\] = cbox:CreateBox\\(\"(.*)\"\\);"));
		if (!cm.empty())
			return line;

		std::regex_match(line, cm, std::regex("    l0\\.(.*) = (.*);"));
		if (cm.size() == 3)
			localVariables[cm[1]] = cm[2];
		else {
			//Lets see if it's an array
			std::regex_match(line, cm, std::regex("    l0\\.(.*) = \\{"));
			if (cm.size() == 2) {
				std::string name = cm[1];
				std::string value = "{";
				//Read Each Entry
				line = readLuaLine(fp);
				while (line != "    };") {
					value += line;
					line = readLuaLine(fp);
				}
				value += "}";
				localVariables[name] = value;
			} else {
				break;
			}
		}

		line = readLuaLine(fp);
	} while (line != "end;");

	return line;
}

std::string DominoFunction::deserialize(FILE *fp) {
	std::string line = readLuaLine(fp);
	do {
		str += line;

		line = readLuaLine(fp);
	} while (line != "end;");

	return line;
}
