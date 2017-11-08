#include "Entity.h"

#include <SDL.h>
#include <unordered_set>
#include "Common.h"
#include "Hash.h"
#include "DDRenderInterface.h"
#include "glm/gtc/matrix_transform.hpp"
#include "xbgFile.h"

std::map<std::string, Node> entityLibrary;
std::unordered_map<uint32_t, std::string> entityLibraryUID;

void addEntity(uint32_t UID, Node &node) {
	Attribute *hidName = node.getAttribute("hidName");
	SDL_assert_release(hidName);

	std::string name = (char*)hidName->buffer.data();
	entityLibrary[name] = node;
	entityLibraryUID[UID] = name;
}

Node* findEntityByUID(uint32_t UID) {
	if (entityLibraryUID.count(UID) > 0)
		return &entityLibrary[entityLibraryUID[UID]];
	return NULL;
}

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

std::unordered_set<std::string> eiBroken;

GLuint generateEntityIcon(Node *entity) {
	char path[500];
	snprintf(path, sizeof(path), "elcache/%s.png", entity->getAttribute("hidName")->buffer.data());
	if (eiBroken.count(path)) return loadResTexture("loading.png");
	GLuint image = loadResTexture(path);
	if (image != 0) return image;

	Node *Components = entity->findFirstChild("Components");
	SDL_assert_release(Components);

	Node* CGraphicComponent = Components->findFirstChild("CGraphicComponent");
	if (CGraphicComponent) {
		Attribute* fileModel = CGraphicComponent->getAttribute("fileModel");

		if (fileModel) {
			auto &model = loadXBG(*(uint32_t*)fileModel->buffer.data());
			if (!model.meshes.empty()) {
				glBindFramebuffer(GL_FRAMEBUFFER, RenderInterface::instance().fbo);
				glClearColor(0.f, 0.f, 0.f, 0.f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				RenderInterface::instance().model.use();
				glm::mat4 MVP = RenderInterface::instance().VP * glm::scale(glm::mat4(), glm::vec3(.5f));
				glUniformMatrix4fv(RenderInterface::instance().model.uniforms["MVP"], 1, GL_FALSE, &MVP[0][0]);
				model.draw();

				char savePath[500];
				snprintf(savePath, sizeof(savePath), "res/elcache/%s.png", entity->getAttribute("hidName")->buffer.data());

				RenderInterface::instance().saveFBO(savePath);
				return loadResTexture(path);
			}
		}
	}

	eiBroken.emplace(path);
	return loadResTexture("loading.png");
}
