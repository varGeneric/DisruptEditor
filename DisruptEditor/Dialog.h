#pragma once

#include "NBCF.h"
#include "locFile.h"
#include <map>
#include <unordered_map>

class Dialog {
public:
	Dialog();
	static Dialog& instance();

	Node conversationtable;
	Node dialogmanagerindices;
	Node speechLength;
	std::map<std::string, Node> behaviortrees;
	std::unordered_map<uint32_t, std::string> locStrings;

	locFile main;
};

