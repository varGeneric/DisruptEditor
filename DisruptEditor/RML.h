#pragma once

#include "tinyxml2.h"
#include <memory>

std::unique_ptr<tinyxml2::XMLDocument> loadRml(const char* filename);


