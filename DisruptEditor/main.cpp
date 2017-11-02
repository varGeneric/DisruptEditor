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
#include <future>
#include <unordered_set>
#include <Ntsecapi.h>

struct BuildingEntity {
	std::string wlu;
	std::string CBatchPath;
	vec3 pos;
	vec3 min, max;
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
				be.pos = swapYZ(*(vec3*)hidPos->buffer.data());

				Node *hidBBox = Entity.findFirstChild("hidBBox");
				be.min = *(vec3*)hidBBox->getAttribute("vectorBBoxMin")->buffer.data();
				be.min = swapYZ(be.min);
				be.min += be.pos;
				be.max = *(vec3*)hidBBox->getAttribute("vectorBBoxMax")->buffer.data();
				be.max = swapYZ(be.max);
				be.max += be.pos;

				buildingEntities.push_back(be);
			}
		}
	}
}

int main(int argc, char **argv) {
	//freopen("debug.log", "wb", stdout);
	SDL_Init(SDL_INIT_EVERYTHING);
	srand(time(NULL));

	LoadingScreen *loadingScreen = new LoadingScreen;
	/*sbaoFile sb;
	sb.open("D:\\Desktop\\bin\\default\\000fac53.sbao");
	return 0;*/

	//Setup INI
	std::string wludir;
	float textDrawDistance = 5.f;
	bool drawBuildings = true;
	std::unordered_map<std::string, bool> windows;
	{
		ini_t* ini = ini_load(loadFile("settings.ini").c_str(), NULL);

		int settings_section = ini_find_section(ini, "settings", 0);
		wludir = ini_property_value(ini, settings_section, ini_find_property(ini, settings_section, "wludir", 0));
		textDrawDistance = atof(ini_property_value(ini, settings_section, ini_find_property(ini, settings_section, "textDrawDistance", 0)));
		drawBuildings = atoi(ini_property_value(ini, settings_section, ini_find_property(ini, settings_section, "displayBuildings", 0)));

		//Pack Files
		int pack_section = ini_find_section(ini, "pack", 0);
		for (int i = 0; i < ini->property_capacity; ++i) {
			if (ini->properties[i].section == pack_section) {
				std::string packPath;
				if (ini->properties[i].value_large)
					packPath = ini->properties[i].value_large;
				else
					packPath = ini->properties[i].value;
				
				if (packPath.back() != '/' && packPath.back() != '\\')
					packPath.push_back('/');

				addSearchPath(packPath);
			}
		}

		ini_destroy(ini);
	}

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
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	
	ImGui_ImplSdlGL3_Init(window);

	//Style
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.19f);
	colors[ImGuiCol_ChildWindowBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_ComboBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.99f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_CloseButton] = ImVec4(0.41f, 0.41f, 0.41f, 0.50f);
	colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
#ifdef IMGUI_HAS_NAV
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.12f);
#endif

	Camera camera;
	camera.type = Camera::FLYCAM;
	camera.far_plane = 4500.f;

	RenderInterface renderInterface;
	dd::initialize(&renderInterface);

	//DominoBox db("D:\\Desktop\\bin\\dlc_solo\\domino\\user\\windycity\\tests\\ai_carfleeing_patterns\\ai_carfleeing_patterns.main.lua");

	Vector< std::future<bool> > tasks;

	loadingScreen->mutex.lock();
	loadingScreen->title = "Loading Language Files...";
	loadingScreen->message.clear();
	loadingScreen->mutex.unlock();
	Dialog::instance();

	tfDIR dir;
	tfDirOpen(&dir, (wludir + std::string("/worlds/windy_city/generated/wlu")).c_str());
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir && strcmp(file.ext, "xml.data.fcb") == 0) {
			SDL_Log("Loading %s\n", file.name);
			wlus[file.name].shortName = file.name;
			//tasks.push_back(std::async(std::launch::async, &wluFile::open, &wlus[file.name], file.path));
			wlus[file.name].open(file.path);

			loadingScreen->mutex.lock();
			loadingScreen->title = "Loading WLUs...";
			loadingScreen->message = file.name;
			loadingScreen->mutex.unlock();
		}
		SDL_PumpEvents();

		tfDirNext(&dir);
	}
	tfDirClose(&dir);

	//Wait for tasks
	/*for (auto &it : tasks) {
		it.wait();
	}
	tasks.clear();*/

	//Scan Materials
	/*tfDirOpen(&dir, "D:/Desktop/bin/windy_city/graphics/_materials");
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir) {
			SDL_Log("Loading %s\n", file.name);

			materialFile spk;
			spk.open(file.path);
		}

		tfDirNext(&dir);
	}
	tfDirClose(&dir);*/

	//Scan Audio
	/*tfDirOpen(&dir, "D:/Desktop/bin/sound_unpack/soundbinary/manual");
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir && strcmp(file.ext, "sbao") == 0) {
			SDL_Log("Loading %s\n", file.name);

			sbaoFile spk;
			spk.open(file.path);
		}

		tfDirNext(&dir);
	}
	tfDirClose(&dir);*/

	{
		loadingScreen->mutex.lock();
		loadingScreen->title = "Loading Entity Library...";
		loadingScreen->message.clear();
		loadingScreen->mutex.unlock();

		FILE *fp = fopen(getAbsoluteFilePath("worlds\\windy_city\\generated\\entitylibrary_rt.fcb").c_str(), "rb");

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

	loadingScreen->mutex.lock();
	loadingScreen->title = "Loading graphics...";
	loadingScreen->message.clear();
	loadingScreen->mutex.unlock();

	Uint32 ticks = SDL_GetTicks();
	uint64_t frameCount = 0;
	std::string currentWlu = wlus.begin()->first;

	bool windowOpen = true;
	while (windowOpen) {
		ImGui_ImplSdlGL3_NewFrame(window);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glBindVertexArray(VertexArrayID);
		glEnable(GL_DEPTH_TEST);

		float delta = (SDL_GetTicks() - ticks) / 1000.f;
		ticks = SDL_GetTicks();
		if (delta > 0.5f)
			delta = 0.5f;

		ivec2 windowSize;
		SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
		glViewport(0, 0, windowSize.x, windowSize.y);
		mat4 view = MATlookAt(camera.location, camera.lookingAt, camera.up);
		mat4 projection = MATperspective(camera.fov, (float)windowSize.x / windowSize.y, camera.near_plane, camera.far_plane);
		mat4 vp = projection * view;
		renderInterface.VP = vp;
		renderInterface.windowSize = windowSize;

		ImGui::SetNextWindowPos(ImVec2(5.f, 5.f));
		ImGui::Begin("##Top", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::DragFloat3("##Camera", (float*)&camera.location);
		if (ImGui::BeginMenu("Assets")) {
			ImGui::PushItemWidth(600.f);
			static char searchBuffer[255] = { '\0' };
			ImGui::InputText("##Search", searchBuffer, sizeof(searchBuffer));

			ImGui::ListBoxHeader("##Entity List");
			for (auto it = entityLibrary.begin(); it != entityLibrary.end(); ++it) {
				Node &entity = it->second;

				int i = entity.children.size();

				std::string name = (const char*)entity.getAttribute("hidName")->buffer.data();
				if (name.find(searchBuffer) != std::string::npos && ImGui::Selectable(name.c_str())) {
					
				}
			}
			ImGui::ListBoxFooter();

			ImGui::PopItemWidth();
			ImGui::EndMenu();
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

					/*it->second.root.findFirstChild("Entities")->children.clear();//DEBUG
					for (auto a = it->second.root.children.begin(); a != it->second.root.children.end();) {
						if (a->getHashName() != "Entities")
							a = it->second.root.children.erase(a);
						else
							++a;
					}*/

					FILE *fp = fopen(it->second.origFilename.c_str(), "wb");
					it->second.serialize(fp);
					fclose(fp);
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools")) {
			if (ImGui::MenuItem("WLU Editor")) {
				windows["Layers"] = true;
			}
			if (ImGui::MenuItem("DARE Converter")) {
			}
			if (ImGui::MenuItem("Domino Editor")) {
			}
			if (ImGui::MenuItem("CSequence Editor")) {
				windows["CSequence"] = true;
			}
			if (ImGui::MenuItem("LocString Editor")) {
				windows["LocString"] = true;
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
			ImGui::DragFloat("Label Draw Distance", &textDrawDistance, 0.05f, 0.f, 4096.f);
			ImGui::Checkbox("Draw Buildings", &drawBuildings);
			ImGui::EndMenu();
		}
		ImGui::End();
		
		if (windows["DARE"] && ImGui::Begin("DARE Converter", &windows["DARE"], 0)) {
			if (ImGui::Button("Convert OGG to SBAO")) {
				const char *src = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "ogg\0*.ogg\0", NULL, NULL);
				const char *dst = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, "sbao\0*.sbao\0", NULL, "00000000");
			}
			ImGui::End();
		}

		if (windows["Domino"] && ImGui::Begin("Domino!", &windows["Domino"], 0)) {
			ImGui::End();
		}

		ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(80.f, 80.f), ImGuiCond_FirstUseEver);
		if (windows["LocString"] && ImGui::Begin("LocString", &windows["LocString"], 0)) {
			

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

				FILE *fp = fopen(wlu.origFilename.c_str(), "wb");
				wlu.serialize(fp);
				fclose(fp);
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

			wlu.drawImGui();

			Node *Entities = wlu.root.findFirstChild("Entities");
			if (!Entities) continue;

			for (Node &entityRef : Entities->children) {
				bool needsCross = true;

				Node *entityPtr = &entityRef;
				Attribute *ArchetypeGuid = entityRef.getAttribute("ArchetypeGuid");
				if (ArchetypeGuid) {
					uint32_t uid = Hash::instance().getFilenameHash((const char*)ArchetypeGuid->buffer.data());
					entityPtr = findEntityByUID(uid);
					SDL_assert_release(entityPtr);
				}
				Node &entity = *entityPtr;

				Attribute *hidName = entity.getAttribute("hidName");
				Attribute *hidPos = entity.getAttribute("hidPos");
				vec3 pos = swapYZ(*(vec3*)hidPos->buffer.data());

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
						mat4 MVP = vp * MATtranslate(mat4(), pos);
						glUniformMatrix4fv(renderInterface.model.uniforms["MVP"], 1, GL_FALSE, &MVP[0][0]);
						model.draw();
					}
				}

				Node *CProximityTriggerComponent = Components->findFirstChild("CProximityTriggerComponent");
				if (CProximityTriggerComponent) {
					needsCross = false;
					vec3 extent = swapYZ(*(vec3*)CProximityTriggerComponent->getAttribute("vectorSize")->buffer.data());
					dd::box(&pos.x, red, extent.x, extent.y, extent.z);
				}

				if (hidBBox && false) {
					vec3 boxMin = swapYZ(*((vec3*)hidBBox->getAttribute("vectorBBoxMin")->buffer.data()));
					vec3 boxMax = swapYZ(*((vec3*)hidBBox->getAttribute("vectorBBoxMax")->buffer.data()));
					vec3 boxExtent = boxMax - boxMin;
					dd::box(&pos.x, blue, boxExtent.x, boxExtent.y, boxExtent.z);
				}

				Node* PatrolDescription = entity.findFirstChild("PatrolDescription");
				if (PatrolDescription) {
					needsCross = false;
					Node* PatrolPointList = PatrolDescription->findFirstChild("PatrolPointList");

					vec3 last;
					for (Node &PatrolPoint : PatrolPointList->children) {
						vec3 pos = swapYZ(*(vec3*)PatrolPoint.getAttribute("vecPos")->buffer.data());

						if (last != vec3())
							dd::line(&last[0], &pos[0], red);
						else
							dd::projectedText((char*)hidName->buffer.data(), &pos.x, red, &vp[0][0], 0, 0, windowSize.x, windowSize.y, 0.5f);
						last = pos;
					}
				}

				Node* RaceDescription = entity.findFirstChild("RaceDescription");
				if (RaceDescription) {
					needsCross = false;
					Node* RacePointList = RaceDescription->findFirstChild("RacePointList");

					vec3 last;
					for (Node &RacePoint : RacePointList->children) {
						vec3 pos = swapYZ(*(vec3*)RacePoint.getAttribute("vecPos")->buffer.data());
						float fShortcutRadius = *(float*)RacePoint.getAttribute("fShortcutRadius")->buffer.data();

						dd::sphere((float*)&pos.x, red, fShortcutRadius);

						if (last != vec3())
							dd::line(&last[0], &pos[0], red);
						else
							dd::projectedText((char*)hidName->buffer.data(), &pos.x, red, &vp[0][0], 0, 0, windowSize.x, windowSize.y, 0.5f);
						last = pos;
					}
				}

				if (pos.distance(camera.location) < textDrawDistance)
					dd::projectedText((char*)hidName->buffer.data(), &pos.x, white, &vp[0][0], 0, 0, windowSize.x, windowSize.y, 0.5f);
				if (needsCross)
					dd::cross(&pos.x, 0.25f);

			}
			ImGui::End();
		}

		//Render Buildings
		if (drawBuildings) {
			if (buildingEntities.empty())
				reloadBuildingEntities();

			renderInterface.model.use();
			for (const BuildingEntity &Entity : buildingEntities) {
				dd::aabb(&Entity.min.x, &Entity.max.x, blue);
				if (Entity.pos.distance(camera.location) < 256)
					dd::projectedText(Entity.wlu.c_str(), &Entity.pos.x, white, &vp[0][0], 0, 0, windowSize.x, windowSize.y, 0.5f);

				mat4 translate = MATtranslate(mat4(), Entity.pos + vec3(0.f, 64.f, 0.f));
				mat4 MVP = vp * MATscale(translate, vec3(128.f));
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

		if (!ImGui::IsAnyItemActive())
			camera.update(delta);

		glBindVertexArray(VertexArrayID);
		dd::flush(0);
		ImGui::Render();
		SDL_GL_SwapWindow(window);
		frameCount++;

		if (loadingScreen && frameCount > 5) {
			SDL_ShowWindow(window);
			delete loadingScreen;
			loadingScreen = NULL;
		}

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
			}
		}
	}

	return 0;
}