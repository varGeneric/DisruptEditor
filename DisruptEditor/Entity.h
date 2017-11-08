#pragma once

#include "NBCF.h"
#include "glad.h"
#include <map>
#include <unordered_map>

extern std::map<std::string, Node> entityLibrary;
extern std::unordered_map<uint32_t, std::string> entityLibraryUID;

void loadEntityLibrary();
Node* findEntityByUID(uint32_t UID);

void drawComponent(Node *entity, Node *node, bool drawImGui, bool draw3D);

GLuint generateEntityIcon(Node *entity);
