#include "glad.h"
#include <SDL.h>
#include "Implementation.h"
#include "Common.h"
#include "DDRenderInterface.h"
#include "spkFile.h"
#include "sbaoFile.h"
#include "cseqFile.h"
#include "wluFile.h"
#include "xbgFile.h"
#include "materialFile.h"
#include "Camera.h"
#include <map>
#include <unordered_map>
#include "Hash.h"
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "LoadingScreen.h"
#include "Dialog.h"
#include "Entity.h"
#include "DominoBox.h"
#include <future>
#include <unordered_set>
#include <Ntsecapi.h>
#include "RML.h"
#include "glm/gtc/matrix_transform.hpp"
#include "CSector.h"
#include "batchFile.h"
#include "Audio.h"

struct BuildingEntity {
	std::string wlu;
	std::string CBatchPath;
	glm::vec3 pos;
	glm::vec3 min, max;
};
Vector<BuildingEntity> buildingEntities;
std::map<std::string, wluFile> wlus;

void reloadBuildingEntities() {
	buildingEntities.clear();
	for (auto it = wlus.begin(); it != wlus.end(); ++it) {
		Node *Entities = it->second.root.findFirstChild("Entities");
		for (Node &Entity : Entities->children) {
			//CBatchMeshEntity
			Attribute *hidEntityClass = Entity.getAttribute("hidEntityClass");
			if (!hidEntityClass) continue;

			if (*(uint32_t*)hidEntityClass->buffer.data() == 138694286) {
				BuildingEntity be;
				be.wlu = it->first;
				be.CBatchPath = (char*)Entity.getAttribute("ExportPath")->buffer.data();

				//Cut off .batch
				be.CBatchPath = be.CBatchPath.substr(0, be.CBatchPath.size() - 6);

				Attribute *hidPos = Entity.getAttribute("hidPos");
				if (!hidPos) continue;
				be.pos = *(glm::vec3*)hidPos->buffer.data();

				Node *hidBBox = Entity.findFirstChild("hidBBox");
				be.min = *(glm::vec3*)hidBBox->getAttribute("vectorBBoxMin")->buffer.data();
				be.min += be.pos;
				be.max = *(glm::vec3*)hidBBox->getAttribute("vectorBBoxMax")->buffer.data();
				be.max += be.pos;

				buildingEntities.push_back(be);
			}
		}
	}
}

int main(int argc, char **argv) {
	SDL_Init(SDL_INIT_EVERYTHING);
	srand(time(NULL));

	std::unordered_map<std::string, bool> windows;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	SDL_Window* window = SDL_CreateWindow(
		"Disrupt Editor",                  // window title
		SDL_WINDOWPOS_CENTERED,           // initial x position
		SDL_WINDOWPOS_CENTERED,           // initial y position
		1600,                               // width, in pixels
		900,                               // height, in pixels
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN // flags - see below
	);
	if (window == NULL) {
		SDL_Log("Could not create window: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	if (glcontext == NULL) {
		SDL_Log("Could not create context: %s\n", SDL_GetError());
		return 1;
	}
	if (SDL_GL_MakeCurrent(window, glcontext) != 0) {
		SDL_Log("Could not create context: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GL_SetSwapInterval(1);
	gladLoadGL();
	
	ImGui_ImplSdlGL3_Init(window);

	//Style
	ImGui::StyleColorsDark(&ImGui::GetStyle());

	Camera camera;
	camera.type = Camera::FLYCAM;
	camera.near_plane = 0.25f;
	camera.far_plane = 6500.f;

	dd::initialize(&RenderInterface::instance());

	//Debug
	/*{
		tfDIR dir;
		/*tfDirOpen(&dir, "D:\\Desktop\\bin\\sound\\soundbinary");
		//tfDirOpen(&dir, "D:\\Desktop\\bin\\sound\\__UNKNOWN/sfx");
		//tfDirOpen(&dir, "D:\\Desktop\\bin\\default");
		while (dir.has_next) {
			tfFILE file;
			tfReadFile(&dir, &file);

			if (!file.is_dir && strstr(file.name, ".sbao") != NULL) {
				SDL_Log("Loading %s\n", file.name);

				sbaoFile bf;
				bf.open(file.path);
			}

			tfDirNext(&dir);
		}
		tfDirClose(&dir);

		tfDirOpen(&dir, "D:\\Desktop\\bin\\windy_city\\worlds\\windy_city\\generated\\batchmeshentity");
		while (dir.has_next) {
			tfFILE file;
			tfReadFile(&dir, &file);

			if (!file.is_dir && strstr(file.name, "_compound.cbatch") != NULL) {
				SDL_Log("Loading %s\n", file.name);

				batchFile bf;
				bf.open(file.path);
			}

			tfDirNext(&dir);
		}
		tfDirClose(&dir);
		

		tfDirOpen(&dir, "D:/Desktop/bin/windy_city/__UNKNOWN/srhr");
		while (dir.has_next) {
			tfFILE file;
			tfReadFile(&dir, &file);

			if (!file.is_dir) {
				SDL_Log("Loading %s\n", file.name);

				CSectorHighRes spk;
				spk.open(file.path);
			}

			tfDirNext(&dir);
		}
		tfDirClose(&dir);
	}*/

	tinyxml2::XMLDocument spawnPointList;

	LoadingScreen *loadingScreen = new LoadingScreen;

	{
		loadingScreen->setTitle("Scanning Files");
		reloadSettings();

		loadingScreen->setTitle("Loading WLUs");
		Vector<FileInfo> files = getFileListFromAbsDir(settings.patchDir + "/worlds/windy_city/generated/wlu", "xml.data.fcb");
		int count = 0;
		for (FileInfo &file : files) {
			SDL_Log("Loading %s\n", file.name.c_str());
			wlus[file.name].shortName = file.name;
			wlus[file.name].open(file.fullPath);
			SDL_PumpEvents();
		}

		if (wlus.empty()) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Disrupt Editor is not configured", "You have not properly setup paths in settings.xml, please see the readme", loadingScreen->getWindow());
			return 0;
		}

		SDL_PumpEvents();
		loadingScreen->setTitle("Loading Entity Library");
		loadEntityLibrary();

		SDL_PumpEvents();
		loadingScreen->setTitle("Loading Language Files");
		Dialog::instance();

		SDL_PumpEvents();
		loadingScreen->setTitle("Loading Particle Library");
		std::unique_ptr<tinyxml2::XMLDocument> particles = loadRml(getAbsoluteFilePath(2646343311).c_str());

		spawnPointList.LoadFile(getAbsoluteFilePath("worlds/windy_city/generated/spawnpointlist.xml").c_str());

		SDL_PumpEvents();
		loadingScreen->setTitle("Loading graphics");
		reloadBuildingEntities();
	}

	/*{
		FILE *out = fopen("out.txt", "w");
		for (auto it = entityLibraryUID.begin(); it != entityLibraryUID.end(); ++it) {
			std::string arche = Hash::instance().getReverseHashFNV(it->first);
			fprintf(out, "%s = %s\n", it->second.c_str(), arche.c_str());
		}
		fclose(out);
	}*/

	/*{
		loadingScreen->mutex.lock();
		loadingScreen->title = "Brute forcing archetypes...";
		loadingScreen->message.clear();
		loadingScreen->mutex.unlock();
		std::unordered_set<uint32_t> unknown;
		{
			for (auto it = entityLibraryUID.begin(); it != entityLibraryUID.end(); ++it) {
				unknown.emplace(it->first);
			}

			for (auto it = Hash::instance().reverseFNVHash.begin(); it != Hash::instance().reverseFNVHash.end(); ++it) {
				unknown.erase(it->first);
			}
		}
		struct UUID {
			uint32_t Data1;
			uint16_t  Data2;
			uint16_t  Data3;
			uint8_t  Data4[8];
		};
		UUID guid;
		char buffer[250];
		uint32_t hash;
		FILE *fp = fopen("res/archeBrute.txt", "a");
		while (!unknown.empty()) {
			snprintf(buffer, sizeof(buffer), "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}", guid.Data1, guid.Data2, guid.Data3,
				guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
				guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

			hash = Hash::instance().getFilenameHash(buffer);
			if (unknown.count(hash) > 0) {
				unknown.erase(hash);
				fprintf(fp, "%s\n", buffer);
				fflush(fp);
			}

			RtlGenRandom(&guid, sizeof(guid));
		}
		fclose(fp);
	}
	return 0;*/

	Uint32 ticks = SDL_GetTicks();
	uint64_t frameCount = 0;
	std::string currentWlu = wlus.begin()->first;

	SDL_ShowWindow(window);
	delete loadingScreen;
	loadingScreen = NULL;

	bool windowOpen = true;
	while (windowOpen) {
		ImGui_ImplSdlGL3_NewFrame(window);
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glBindVertexArray(RenderInterface::instance().VertexArrayID);
		glEnable(GL_DEPTH_TEST);

		float delta = (SDL_GetTicks() - ticks) / 1000.f;
		ticks = SDL_GetTicks();
		if (delta > 0.5f)
			delta = 0.5f;

		RenderInterface &renderInterface = RenderInterface::instance();

		SDL_GetWindowSize(window, &renderInterface.windowSize.x, &renderInterface.windowSize.y);
		glViewport(0, 0, renderInterface.windowSize.x, renderInterface.windowSize.y);
		renderInterface.View = glm::lookAtLH(camera.location, camera.lookingAt, camera.up);
		renderInterface.Projection = glm::perspective(camera.fov, (float)renderInterface.windowSize.x / renderInterface.windowSize.y, camera.near_plane, camera.far_plane);
		renderInterface.VP = renderInterface.Projection * renderInterface.View;

		ImGui::BeginMainMenuBar();
		if (ImGui::MenuItem("Entity Library")) {
			windows["EntityLibrary"] ^= true;
		}
		if (ImGui::MenuItem("Terrain")) {
			windows["Terrain"] ^= true;
		}
		if (ImGui::MenuItem("Domino")) {
			windows["Domino"] ^= true;
		}
		if (ImGui::BeginMenu("Batch")) {
			if (ImGui::MenuItem("Import Wlu XML")) {
				for (auto it = wlus.begin(); it != wlus.end(); ++it) {
					std::string xmlFileName = it->second.shortName + ".xml";
					tinyxml2::XMLDocument doc;
					doc.LoadFile(xmlFileName.c_str());
					it->second.root.deserializeXML(doc.RootElement());
				}
			}
			if (ImGui::MenuItem("Export Wlu XML")) {
				for (auto it = wlus.begin(); it != wlus.end(); ++it) {
					std::string xmlFileName = it->second.shortName + ".xml";
					FILE *fp = fopen(xmlFileName.c_str(), "wb");
					tinyxml2::XMLPrinter printer(fp);
					it->second.root.serializeXML(printer);
					fclose(fp);
				}
			}
			if (ImGui::MenuItem("Save Wlu")) {
				for (auto it = wlus.begin(); it != wlus.end(); ++it) {
					std::string backup = it->second.origFilename;
					backup += ".bak";
					CopyFileA(it->second.origFilename.c_str(), backup.c_str(), TRUE);

					it->second.root.findFirstChild("Entities")->children.clear();//DEBUG
					for (auto a = it->second.root.children.begin(); a != it->second.root.children.end();) {
						if (a->getHashName() != "Entities")
							a = it->second.root.children.erase(a);
						else
							++a;
					}

					it->second.serialize(it->second.origFilename.c_str());
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools")) {
			if (ImGui::MenuItem("DARE Converter")) {
				windows["DARE"] ^= true;
			}
			if (ImGui::MenuItem("CSequence Editor")) {
				windows["CSequence"] ^= true;
			}
			if (ImGui::MenuItem("LocString Editor")) {
				windows["LocString"] ^= true;
			}
			if (ImGui::MenuItem("SpawnPoint Editor")) {
				windows["SpawnPoint"] ^= true;
			}
			if (ImGui::BeginMenu("Hasher")) {
				static char buffer[255] = { '\0' };
				ImGui::InputText("##UID", buffer, sizeof(buffer));
				uint32_t fnv = Hash::instance().getFilenameHash(buffer);
				uint32_t crc = Hash::instance().getHash(buffer);

				char outbuffer[255];
				snprintf(outbuffer, sizeof(outbuffer), "%u", fnv);
				ImGui::InputText("FNV##UIDOUT", outbuffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
				snprintf(outbuffer, sizeof(outbuffer), "%u", crc);
				ImGui::InputText("CRC##UIDOUT", outbuffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings")) {
			ImGui::DragFloat("Camera Fly Multiplier", &camera.flyMultiplier, 0.02f, 1.f, 10.f);
			ImGui::DragFloat("Label Draw Distance", &settings.textDrawDistance, 0.05f, 0.f, 4096.f);
			ImGui::Checkbox("Draw Buildings", &settings.drawBuildings);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
		
		ImGui::SetNextWindowSize(ImVec2(1000, 360), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(7.f, 537.f), ImGuiCond_FirstUseEver);
		if (windows["EntityLibrary"] && ImGui::Begin("Entity Library", &windows["EntityLibrary"], 0)) {
			//ImGui::PushItemWidth(-1.f);
			static char searchBuffer[255] = { '\0' };
			ImGui::InputText("##Search", searchBuffer, sizeof(searchBuffer));

			ImVec2 resSize = ImGui::GetContentRegionAvail();
			float left = resSize.x;

			//ImGui::ListBoxHeader("##Entity List", resSize);
			for (auto it = entityLibrary.begin(); it != entityLibrary.end(); ++it) {
				Node &entity = it->second;

				int i = entity.children.size();

				std::string name = (const char*)entity.getAttribute("hidName")->buffer.data();
				if (name.find(searchBuffer) != std::string::npos) {
					ImGui::PushID(name.c_str());
					ImGui::Image((ImTextureID)generateEntityIcon(&entity), ImVec2(128.f, 128.f));
					ImGui::PopID();

					if(ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", name.c_str());

					left -= ImGui::GetItemRectSize().x;
					if (left > ImGui::GetItemRectSize().x + 60.f)
						ImGui::SameLine();
					else
						left = resSize.x;
				}
			}
			//ImGui::ListBoxFooter();
			//ImGui::PopItemWidth();
			ImGui::End();
		}

		if (windows["DARE"] && ImGui::Begin("DARE Converter", &windows["DARE"], 0)) {
			static sbaoFile file;
			static std::string currentFile;
			static int currentSound = 0;
			if (ImGui::Button("Open")) {
				currentFile = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "sbao\0*.sbao\0", NULL, NULL);
				file.open(currentFile.c_str());
			}

			int layerNum = 1;
			for (auto it = file.layers.begin(); it != file.layers.end(); ++it) {
				ImGui::PushID(it);
				ImGui::Text("%u", layerNum);
				ImGui::SameLine();
				if (ImGui::Button("Play")) {
					Audio::instance().stopSound(currentSound);
					it->play(false);
				}

				ImGui::PopID();
				layerNum++;
			}

			ImGui::End();
		}

		if (windows["Domino"] && ImGui::Begin("Domino!", &windows["Domino"], ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
			static DominoBox db;
			ImGui::BeginMenuBar();
			if (ImGui::MenuItem("Open")) {
			}
			if (ImGui::MenuItem("Save")) {
			}
			if (ImGui::MenuItem("Save As")) {
			}
			if (ImGui::MenuItem("Import Lua")) {
				const char *currentFile = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, NULL, NULL, NULL);
				db.open(currentFile);
			}
			if (ImGui::MenuItem("Export Lua")) {
				const char *currentFile = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, NULL, NULL, NULL);
			}
			ImGui::EndMenuBar();

			db.draw();

			ImGui::End();
		}

		ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(80.f, 80.f), ImGuiCond_FirstUseEver);
		if (windows["LocString"] && ImGui::Begin("LocString", &windows["LocString"], 0)) {
			

			ImGui::End();
		}

		ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(80.f, 80.f), ImGuiCond_FirstUseEver);
		if (windows["SpawnPoint"] && ImGui::Begin("SpawnPoint List", &windows["SpawnPoint"], 0)) {
			for (tinyxml2::XMLElement *SpawnPoint = spawnPointList.RootElement()->FirstChildElement(); SpawnPoint; SpawnPoint = SpawnPoint->NextSiblingElement()) {
				//glm::vec3 Position = SpawnPoint->Attribute("Position");
			}

			ImGui::End();
		}

		ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(80.f, 80.f), ImGuiCond_FirstUseEver);
		if (windows["CSequence"] && ImGui::Begin("CSequence", &windows["CSequence"], 0)) {
			static std::string currentFile;
			static cseqFile file;
			ImGui::Text("%s", currentFile.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Open")) {
				currentFile = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "cseq\0*.cseq\0", getAbsoluteFilePath("sequences").c_str(), NULL);
				file.open(currentFile.c_str());
			}

			ImGui::End();
		}

		//Draw Layer Window
		ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(1150.f, 5.f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Layers")) {
			//Wlu List
			ImGui::PushItemWidth(-1.f);
			static char searchWluBuffer[255] = { 0 };
			ImGui::InputText("##Search", searchWluBuffer, sizeof(searchWluBuffer));

			ImVec2 size = ImGui::GetWindowContentRegionMax();
			size.y -= 75;
			size.x -= 5;
			ImGui::ListBoxHeader("##WLU List", size);
			for (auto it = wlus.begin(); it != wlus.end(); ++it) {
				if (it->first.find(searchWluBuffer) != std::string::npos) {
					bool selected = currentWlu == it->first;
					if (ImGui::Selectable(it->first.c_str(), selected))
						currentWlu = it->first;
				}
			}
			ImGui::ListBoxFooter();
			ImGui::PopItemWidth();

			wluFile &wlu = wlus[currentWlu];

			if (ImGui::Button("Save")) {
				std::string backup = wlu.origFilename;
				backup += ".bak";
				CopyFileA(wlu.origFilename.c_str(), backup.c_str(), TRUE);
				wlu.serialize(wlu.origFilename.c_str());
			}
			ImGui::SameLine();
			if (ImGui::Button("Reload")) {
				SDL_assert_release(wlu.open(wlu.origFilename.c_str()));
			}
			ImGui::SameLine();
			if (ImGui::Button("Restore")) {
				std::string backup = wlu.origFilename;
				backup += ".bak";
				CopyFileA(backup.c_str(), wlu.origFilename.c_str(), FALSE);
				wlu.open(wlu.origFilename.c_str());
			}
			ImGui::SameLine();
			std::string xmlFileName = wlu.shortName + ".xml";
			if (ImGui::Button("XML")) {
				FILE *fp = fopen(xmlFileName.c_str(), "wb");
				tinyxml2::XMLPrinter printer(fp);
				wlu.root.serializeXML(printer);
				fclose(fp);
			}
			ImGui::SameLine();
			if (ImGui::Button("Import XML")) {
				tinyxml2::XMLDocument doc;
				doc.LoadFile(xmlFileName.c_str());
				wlu.root.deserializeXML(doc.RootElement());
			}
			if (wlu.bailOut)
				ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "This file could not be completely read, do not save it!");
			
			ImGui::End();
			ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
			ImGui::Begin("Properties");

			wlu.draw();

			Node *Entities = wlu.root.findFirstChild("Entities");
			if (!Entities) continue;

			for (Node &entityRef : Entities->children) {
				bool needsCross = true;

				Node *entityPtr = &entityRef;
				Attribute *ArchetypeGuid = entityRef.getAttribute("ArchetypeGuid");
				if (ArchetypeGuid) {
					uint32_t uid = Hash::instance().getFilenameHash((const char*)ArchetypeGuid->buffer.data());
					entityPtr = findEntityByUID(uid);
					if (!entityPtr) {
						SDL_Log("Could not find %s\n", ArchetypeGuid->buffer.data());
						SDL_assert_release(false && "Could not lookup entity by archtype, check that dlc_solo is loaded first before other packfiles");
						entityPtr = &entityRef;
					}
				}
				Node &entity = *entityPtr;

				Attribute *hidName = entity.getAttribute("hidName");
				Attribute *hidPos = entity.getAttribute("hidPos");
				glm::vec3 pos = *(glm::vec3*)hidPos->buffer.data();

				//
				Node *hidBBox = entity.findFirstChild("hidBBox");

				Node *Components = entity.findFirstChild("Components");
				SDL_assert_release(Components);

				Node* CGraphicComponent = Components->findFirstChild("CGraphicComponent");
				if (CGraphicComponent) {
					Attribute* XBG = CGraphicComponent->getAttribute(0x3182766C);

					if (XBG && XBG->buffer.size() > 5) {
						auto &model = loadXBG((char*)XBG->buffer.data());
						renderInterface.model.use();
						glm::mat4 MVP = renderInterface.VP * glm::translate(glm::mat4(), pos);
						glUniformMatrix4fv(renderInterface.model.uniforms["MVP"], 1, GL_FALSE, &MVP[0][0]);
						model.draw();
					}
				}

				Node *CProximityTriggerComponent = Components->findFirstChild("CProximityTriggerComponent");
				if (CProximityTriggerComponent) {
					needsCross = false;
					glm::vec3 extent = *(glm::vec3*)CProximityTriggerComponent->getAttribute("vectorSize")->buffer.data();
					dd::box(&pos.x, red, extent.x, extent.y, extent.z);
				}

				if (hidBBox && false) {
					glm::vec3 boxMin = *((glm::vec3*)hidBBox->getAttribute("vectorBBoxMin")->buffer.data());
					glm::vec3 boxMax = *((glm::vec3*)hidBBox->getAttribute("vectorBBoxMax")->buffer.data());
					glm::vec3 boxExtent = boxMax - boxMin;
					dd::box(&pos.x, blue, boxExtent.x, boxExtent.y, boxExtent.z);
				}

				Node* PatrolDescription = entity.findFirstChild("PatrolDescription");
				if (PatrolDescription) {
					needsCross = false;
					Node* PatrolPointList = PatrolDescription->findFirstChild("PatrolPointList");

					glm::vec3 last;
					for (Node &PatrolPoint : PatrolPointList->children) {
						glm::vec3 pos = *(glm::vec3*)PatrolPoint.getAttribute("vecPos")->buffer.data();

						if (last != glm::vec3())
							dd::line(&last[0], &pos[0], red);
						else
							dd::projectedText((char*)hidName->buffer.data(), &pos.x, red, &renderInterface.VP[0][0], 0, 0, renderInterface.windowSize.x, renderInterface.windowSize.y, 0.5f);
						last = pos;
					}
				}

				Node* RaceDescription = entity.findFirstChild("RaceDescription");
				if (RaceDescription) {
					needsCross = false;
					Node* RacePointList = RaceDescription->findFirstChild("RacePointList");

					glm::vec3 last;
					for (Node &RacePoint : RacePointList->children) {
						glm::vec3 pos = *(glm::vec3*)RacePoint.getAttribute("vecPos")->buffer.data();
						float fShortcutRadius = *(float*)RacePoint.getAttribute("fShortcutRadius")->buffer.data();

						dd::sphere((float*)&pos.x, red, fShortcutRadius);

						if (last != glm::vec3())
							dd::line(&last[0], &pos[0], red);
						else
							dd::projectedText((char*)hidName->buffer.data(), &pos.x, red, &renderInterface.VP[0][0], 0, 0, renderInterface.windowSize.x, renderInterface.windowSize.y, 0.5f);
						last = pos;
					}
				}

				if (glm::distance(pos, camera.location) < settings.textDrawDistance)
					dd::projectedText((char*)hidName->buffer.data(), &pos.x, white, &renderInterface.VP[0][0], 0, 0, renderInterface.windowSize.x, renderInterface.windowSize.y, 0.5f);
				if (needsCross)
					dd::cross(&pos.x, 0.25f);

			}
			ImGui::End();
		}

		//Render Buildings
		if (settings.drawBuildings) {
			renderInterface.model.use();
			for (const BuildingEntity &Entity : buildingEntities) {
				dd::aabb(&Entity.min.x, &Entity.max.x, blue);
				if (glm::distance(Entity.pos, camera.location) < 256)
					dd::projectedText(Entity.wlu.c_str(), &Entity.pos.x, white, &renderInterface.VP[0][0], 0, 0, renderInterface.windowSize.x, renderInterface.windowSize.y, 0.5f);

				glm::mat4 translate = glm::translate(glm::mat4(), Entity.pos + glm::vec3(0.f, 64.f, 0.f));
				glm::mat4 MVP = renderInterface.VP * glm::scale(translate, glm::vec3(128.f));
				glUniformMatrix4fv(renderInterface.model.uniforms["MVP"], 1, GL_FALSE, &MVP[0][0]);

				{
					std::string CBatchXbgPath = Entity.CBatchPath + "_building_low.xbg";
					auto &model = loadXBG(CBatchXbgPath);
					if (model.meshes.empty()) continue;
					model.draw();
				}

				//Draw roof
				{
					std::string CBatchXbgPath = Entity.CBatchPath + "_building_roofs.xbg";
					auto &model = loadXBG(CBatchXbgPath);
					model.draw();
				}
			}
		}

		if (!ImGui::IsAnyWindowHovered())
			camera.update(delta);

		glBindVertexArray(RenderInterface::instance().VertexArrayID);
		dd::flush(0);
		ImGui::Render();
		SDL_GL_SwapWindow(window);
		frameCount++;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSdlGL3_ProcessEvent(&event);
			switch (event.type) {
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
						windowOpen = false;
						exit(0);
					}
					break;
				case SDL_DROPFILE:
					Node node = readFCB(event.drop.file);
					std::string out = event.drop.file + std::string(".xml");
					FILE *fp = fopen(out.c_str(), "w");
					tinyxml2::XMLPrinter printer(fp);
					node.serializeXML(printer);
					fclose(fp);
					break;
			}
		}
	}

	return 0;
}