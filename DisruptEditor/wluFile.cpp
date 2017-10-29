#include "wluFile.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "tinyxml2.h"
#include "Hash.h"
#include "Common.h"
#include "imgui.h"
#include "Implementation.h"

static inline void seekpad(FILE *fp, long pad) {
	//16-byte chunk alignment
	long size = ftell(fp);
	long seek = (pad - (size % pad)) % pad;
	fseek(fp, seek, SEEK_CUR);
}

static inline void writepad(FILE *fp, long pad) {
	//16-byte chunk alignment
	long size = ftell(fp);
	long seek = (pad - (size % pad)) % pad;

	uint8_t zero[32] = { 0 };
	fwrite(zero, 1, seek, fp);
}

bool wluFile::open(std::string filename) {
	origFilename = filename;
	FILE *fp = fopen(filename.c_str(), "rb");
	if (!fp) {
		return false;
	}

	fread(&wluhead, sizeof(wluhead), 1, fp);

	assert(memcmp(wluhead.base.magic, "ESAB", 4) == 0);
	assert(wluhead.base.unknown1 == 3 || wluhead.base.unknown1 == 0 || wluhead.base.unknown1 == 1 || wluhead.base.unknown1 == 2);
	assert(wluhead.base.unknown2 == 0);
	assert(memcmp(wluhead.fcb.magic, "nbCF", 4) == 0);
	assert(wluhead.fcb.version == 16389);
	assert(wluhead.fcb.headerFlags == 0);
	assert(wluhead.fcb.totalObjectCount == wluhead.fcb.totalValueCount + 1);

	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp) - sizeof(wluhead.base);
	fseek(fp, sizeof(wluhead), SEEK_SET);

	//Pad size to 4 bytes
	//TODO Figure out size
	//assert(wluhead.base.size == size || wluhead.base.size == size-1 || wluhead.base.size == size - 2 || wluhead.base.size == size - 3);

	//2296 size
	//2265 wlu base size + 16

	bailOut = false;
	root.deserialize(fp, bailOut);

	fseek(fp, wluhead.base.size + sizeof(wluhead.base), SEEK_SET);
	seekpad(fp, 4);

	size_t offset = ftell(fp);
	if (offset != size + sizeof(wluhead.base)) {
		handleHeaders(fp, size + sizeof(wluhead.base));

		offset = ftell(fp);
		assert(offset == size + sizeof(wluhead.base));
	}

	fclose(fp);

	return !bailOut;
}

void wluFile::handleHeaders(FILE * fp, size_t size) {
	//Read Magic
	char magic[5];
	fread(magic, 4, 1, fp);
	magic[4] = '\0';
	fseek(fp, -4, SEEK_CUR);

	//LAUQ - LoadQuality
	//ROAD - 

	if (magic == std::string("DAOR")) {
		roadHeader road;
		fread(&road, sizeof(road), 1, fp);
		fseek(fp, road.size, SEEK_CUR);
		printf("Road %i\n", road.size);
		seekpad(fp, 16);
	} else if (magic == std::string("LAUQ")) {
		qualityHeader qual;
		fread(&qual, sizeof(qual), 1, fp);
		fseek(fp, qual.size, SEEK_CUR);
		printf("Qual %i\n", qual.size);
	} else {
		size_t offset = ftell(fp);
		assert(false);
	}

	if (ftell(fp) != size) {
		handleHeaders(fp, size);
	}
}

void wluFile::serialize(FILE *fp) {
	memcpy(wluhead.base.magic, "ESAB", 4);
	//wluhead.base.unknown1 = wluhead.base.unknown2 = 0;

	memcpy(wluhead.fcb.magic, "nbCF", 4);
	wluhead.fcb.version = 16389;
	wluhead.fcb.totalObjectCount = 1 + root.countNodes();
	wluhead.fcb.totalValueCount = wluhead.fcb.totalObjectCount - 1;

	fwrite(&wluhead, sizeof(wluhead), 1, fp);

	root.serialize(fp);

	fseek(fp, 0, SEEK_END);
	writepad(fp, 4);
	wluhead.base.size = ftell(fp) - sizeof(wluhead.base);
	fseek(fp, 0, SEEK_SET);
	fwrite(&wluhead, sizeof(wluhead), 1, fp);
}

void wluFile::drawImGui() {
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

	if(selectedEntity) {
		ImGui::Separator();
		Node &entity = *selectedEntity;

		char imguiHash[512];
		Attribute *hidName = entity.getAttribute("hidName");
		Attribute *hidPos = entity.getAttribute("hidPos");
		vec3 pos = swapYZ(*(vec3*)hidPos->buffer.data());

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

		Node *CGraphicComponent = Components->findFirstChild("CGraphicComponent");
		if (CGraphicComponent && ImGui::TreeNode("CGraphicComponent")) {

			Attribute *_3182766c = CGraphicComponent->getAttribute(0x3182766c);
			ImGui::Text("_3182766c: %s", _3182766c->buffer.data());

			ImGui::TreePop();
		}

		Node* PatrolDescription = entity.findFirstChild("PatrolDescription");
		if (PatrolDescription && ImGui::TreeNode("PatrolDescription")) {
			Node* PatrolPointList = PatrolDescription->findFirstChild("PatrolPointList");
			for (Node &PatrolPoint : PatrolPointList->children) {
				ImGui::InputFloat3("##a", (float*)PatrolPoint.getAttribute("vecPos")->buffer.data());
			}
			ImGui::TreePop();
		}

	}
}
