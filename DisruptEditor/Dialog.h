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
	std::map<int32_t, std::string> locStrings;
	std::unordered_map<int32_t, uint32_t> soundidlinelinks;

	//locFile main;
};

