#include "wluFile.h"

#include <stdio.h>
#include <string.h>
#include <SDL_assert.h>
#include <SDL_log.h>
#include <stdlib.h>

#include "tinyxml2.h"
#include "Hash.h"
#include "Common.h"
#include "imgui.h"
#include "Implementation.h"
#include "Entity.h"

static inline void seekpad(SDL_RWops *fp, long pad) {
	//16-byte chunk alignment
	long size = SDL_RWtell(fp);
	long seek = (pad - (size % pad)) % pad;
	SDL_RWseek(fp, seek, RW_SEEK_CUR);
}

static inline void writepad(SDL_RWops *fp, long pad) {
	//16-byte chunk alignment
	long size = SDL_RWtell(fp);
	long seek = (pad - (size % pad)) % pad;

	uint8_t zero[32] = { 0 };
	SDL_RWwrite(fp, zero, 1, seek);
}

bool wluFile::open(std::string filename) {
	origFilename = filename;
	SDL_RWops *fp = SDL_RWFromFile(filename.c_str(), "rb");
	if (!fp) {
		return false;
	}

	Vector<uint8_t> data(SDL_RWsize(fp));
	SDL_RWread(fp, data.data(), data.size(), 1);
	SDL_RWclose(fp);
	fp = SDL_RWFromConstMem(data.data(), data.size());

	uint32_t magic = SDL_ReadLE32(fp);
	SDL_RWseek(fp, 0, RW_SEEK_SET);
	bool ret;
	if (magic == 1111577413) {
		//WD1 File
		ret = openWD1(fp);
		isWD2 = false;
	} else if (magic == 4129362901) {
		//WD2 File
		ret = openWD2(fp);
		isWD2 = true;
	}

	SDL_RWclose(fp);
}

bool wluFile::openWD1(SDL_RWops *fp) {
	SDL_RWread(fp, &wluhead, sizeof(wluhead), 1);

	SDL_assert_release(wluhead.magic == 1111577413);
	SDL_assert_release(wluhead.unknown1 == 3 || wluhead.unknown1 == 0 || wluhead.unknown1 == 1 || wluhead.unknown1 == 2);
	SDL_assert_release(wluhead.unknown2 == 0);

	SDL_RWseek(fp, 0, RW_SEEK_END);
	size_t size = SDL_RWtell(fp) - sizeof(wluhead);
	SDL_RWseek(fp, sizeof(wluhead), RW_SEEK_SET);

	//Pad size to 4 bytes
	//TODO Figure out size
	//SDL_assert_release(wluhead.base.size == size || wluhead.base.size == size-1 || wluhead.base.size == size - 2 || wluhead.base.size == size - 3);

	//2296 size
	//2265 wlu base size + 16

	bailOut = false;
	root.deserialize(fp, bailOut);

	SDL_RWseek(fp, wluhead.size + sizeof(wluhead), RW_SEEK_SET);
	seekpad(fp, 4);

	size_t offset = SDL_RWtell(fp);
	size_t extraBegin = offset;
	if (offset != size + sizeof(wluhead)) {
		handleHeaders(fp, size + sizeof(wluhead));

		offset = SDL_RWtell(fp);
		SDL_assert_release(offset == size + sizeof(wluhead));
	}

	//Read in Extra Data
	extraData.resize(size + sizeof(wluhead) - extraBegin);
	if (!extraData.empty()) {
		SDL_RWseek(fp, extraBegin, RW_SEEK_SET);
		SDL_RWread(fp, extraData.data(), 1, extraData.size());
	}

	//Handle .embed
	/*fp = fopen((filename + ".embed").c_str(), "rb");
	if (fp) {
	uint32_t magic, size;
	fread(&magic, sizeof(magic), 1, fp);
	fread(&size, sizeof(size), 1, fp);

	fseek(fp, 1, SEEK_CUR);
	fseek(fp, size * 37, SEEK_CUR);
	SDL_assert_release(feof(fp));

	fclose(fp);
	}*/

	return !bailOut;
}

bool wluFile::openWD2(SDL_RWops * fp) {
	SDL_RWread(fp, &wluhead, sizeof(wluhead), 1);

	SDL_assert_release(wluhead.magic == 4129362901);
	SDL_assert_release(wluhead.unknown2 == 0);

	root = readFCB(fp);

	return true;
}

void wluFile::handleHeaders(SDL_RWops * fp, size_t size) {
	//Read Magic
	char magic[5];
	SDL_RWread(fp, magic, 4, 1);
	magic[4] = '\0';
	SDL_RWseek(fp, -4, RW_SEEK_CUR);

	//LAUQ - LoadQuality
	//ROAD - 

	if (magic == std::string("DAOR")) {
		roadHeader road;
		SDL_RWread(fp, &road, sizeof(road), 1);
		SDL_RWseek(fp, road.size, RW_SEEK_CUR);
		SDL_Log("Road %i\n", road.size);
		seekpad(fp, 16);
	} else if (magic == std::string("LAUQ")) {
		qualityHeader qual;
		SDL_RWread(fp, &qual, sizeof(qual), 1);
		SDL_RWseek(fp, qual.size, RW_SEEK_CUR);
		SDL_Log("Qual %i\n", qual.size);
	} else {
		size_t offset = SDL_RWtell(fp);
		SDL_assert_release(false);
	}

	if (SDL_RWtell(fp) != size) {
		handleHeaders(fp, size);
	}
}

void wluFile::serialize(const char* filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "wb");
	
	if (isWD2) {
		SDL_RWwrite(fp, &wluhead, sizeof(wluhead), 1);
		writeFCBB(fp, root);
		wluhead.size = SDL_RWtell(fp) - sizeof(wluhead);
		writepad(fp, 16);
		SDL_RWseek(fp, 0, RW_SEEK_SET);
		SDL_RWwrite(fp, &wluhead, sizeof(wluhead), 1);
	} else {
		wluhead.magic = 1111577413;
		//wluhead.base.unknown1 = wluhead.base.unknown2 = 0;

		SDL_RWwrite(fp, &wluhead, sizeof(wluhead), 1);

		writeFCBB(fp, root);

		SDL_RWseek(fp, 0, RW_SEEK_END);
		writepad(fp, 4);
		wluhead.size = SDL_RWtell(fp) - sizeof(wluhead);

		//Write Extra Data
		//fwrite(extraData.data(), 1, extraData.size(), fp);

		SDL_RWseek(fp, 0, RW_SEEK_SET);
		SDL_RWwrite(fp, &wluhead, sizeof(wluhead), 1);
	}
	
	SDL_RWclose(fp);
}

void wluFile::draw(bool drawImgui, bool draw3D) {
	Node *Entities = root.findFirstChild("Entities");
	if (!Entities) return;

	//Draw List of Entites
	ImGui::PushItemWidth(-1.f);
	static char searchWluBuffer[255] = { 0 };
	ImGui::InputText("##Search", searchWluBuffer, sizeof(searchWluBuffer));
	ImGui::ListBoxHeader("##Entity List");
	for (Node &entity : Entities->children) {
		Attribute *hidName = entity.getAttribute("hidName");

		char tempName[255] = { '\0' };
		snprintf(tempName, sizeof(tempName), "%s##%p", hidName->buffer.data(), &entity);

		if (std::string(tempName).find(searchWluBuffer) == std::string::npos) continue;

		bool selected = &entity == selectedEntity;
		if (ImGui::Selectable(tempName, selected))
			selectedEntity = &entity;
	}
	ImGui::ListBoxFooter();
	ImGui::PopItemWidth();

	char imGuiBuffer[1024];

	if(selectedEntity) {
		ImGui::Separator();
		Node &entity = *selectedEntity;

		char imguiHash[512];
		Attribute *hidName = entity.getAttribute("hidName");
		Attribute *hidPos = entity.getAttribute("hidPos");
		glm::vec3 pos = *(glm::vec3*)hidPos->buffer.data();

		//Iterate through Entity Attributes
		snprintf(imguiHash, sizeof(imguiHash), "%s##%p", hidName->buffer.data(), &entity);
		
		dd::sphere((float*)&pos, red, 5.f, 0, false);

		Attribute *ArchetypeGuid = entity.getAttribute("ArchetypeGuid");
		if (ArchetypeGuid) {
			uint32_t uid = Hash::instance().getFilenameHash((const char*)ArchetypeGuid->buffer.data());
			char temp[40];
			snprintf(temp, sizeof(temp), "UID: %u", uid);
			if (ImGui::Selectable(temp)) {
				snprintf(temp, sizeof(temp), "%u", uid);
				ImGui::SetClipboardText(temp);
			}
		}

		for (Attribute &attr : entity.attributes) {
			char name[1024];
			snprintf(name, sizeof(name), "%s##%p", Hash::instance().getReverseHash(attr.hash).c_str(), &attr);

			Hash::Types type = Hash::instance().getHashType(attr.hash);

			switch (type) {
				case Hash::STRING:
				{
					char temp[1024];
					strncpy(temp, (char*)attr.buffer.data(), sizeof(temp));
					if (ImGui::InputText(name, temp, sizeof(temp))) {
						attr.buffer.resize(strlen(temp) + 1);
						strcpy((char*)attr.buffer.data(), temp);
					}
					break;
				}
				case Hash::FLOAT:
					ImGui::DragFloat(name, (float*)attr.buffer.data());
					break;
				case Hash::UINT64:
					ImGui::InputUInt64(name, (uint64_t*)attr.buffer.data());
					break;
				case Hash::VEC2:
					ImGui::DragFloat2(name, (float*)attr.buffer.data());
					break;
				case Hash::VEC3:
					ImGui::DragFloat3(name, (float*)attr.buffer.data());
					break;
				case Hash::VEC4:
					ImGui::DragFloat4(name, (float*)attr.buffer.data());
					break;
				default:
					ImGui::LabelText(name, "BinHex %u", attr.buffer.size());
					break;
			}
		}

		//Handle Components
		Node* Components = entity.findFirstChild("Components");
		if (Components) {
			for (Node &it : Components->children) {
				drawComponent(&entity, &it, true, true);
			}
		}

		Node* PatrolDescription = entity.findFirstChild("PatrolDescription");
		if (PatrolDescription && ImGui::TreeNode("PatrolDescription")) {
			Node* PatrolPointList = PatrolDescription->findFirstChild("PatrolPointList");
			for (Node &PatrolPoint : PatrolPointList->children) {
				snprintf(imGuiBuffer, sizeof(imGuiBuffer), "##%p", PatrolPoint);
				ImGui::DragFloat3(imGuiBuffer, (float*)PatrolPoint.getAttribute("vecPos")->buffer.data());
			}
			ImGui::TreePop();
		}

	}
}
