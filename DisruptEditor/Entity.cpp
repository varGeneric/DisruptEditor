#include "Entity.h"

#include <SDL.h>
#include "Common.h"
#include "Hash.h"

void loadEntityLibrary() {
	FILE *fp = fopen(getAbsoluteFilePath("worlds\\windy_city\\generated\\entitylibrary_rt.fcb").c_str(), "rb");
	SDL_assert_release(fp);

	uint32_t infoOffset, infoCount;
	fread(&infoOffset, sizeof(infoOffset), 1, fp);
	fread(&infoCount, sizeof(infoCount), 1, fp);
	fseek(fp, infoOffset, SEEK_SET);

	for (uint32_t i = 0; i < infoCount; ++i) {
		uint32_t UID, offset;
		fread(&UID, sizeof(UID), 1, fp);
		fread(&offset, sizeof(offset), 1, fp);
		offset += 8;

		size_t curOffset = ftell(fp) + 4;

		fseek(fp, offset, SEEK_SET);
		bool bailOut = false;
		Node entityParent;
		entityParent.deserialize(fp, bailOut);
		SDL_assert_release(!bailOut);
		addEntity(UID, *entityParent.children.begin());

		fseek(fp, curOffset, SEEK_SET);
	}
	fclose(fp);
}

void drawCGraphicComponent(Node *entity, Node *node, bool drawImGui, bool draw3D) {
	
}

#define handleComponent(componentName) else if(name == #componentName) draw##componentName(entity, node, drawImGui, draw3D);

void drawComponent(Node *entity, Node *node, bool drawImGui, bool draw3D) {
	std::string name = Hash::instance().getReverseHash(node->hash);

	if (false) {

	}
	handleComponent(CGraphicComponent)
	else {
		if (drawImGui)
			ImGui::Text("Unimplemented Component: %s", name.c_str());
	}
}
