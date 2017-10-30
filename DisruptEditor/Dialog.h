#pragma once

#include "NBCF.h"
#include <map>

class Dialog {
public:
	Dialog();
	static Dialog& instance();

	Node conversationtable;
	Node dialogmanagerindices;
	std::map<std::string, Node> behaviortrees;
};

