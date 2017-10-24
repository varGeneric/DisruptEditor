#include "glad.h"
#include <SDL.h>
#include "Implementation.h"
#include "Common.h"
#include "DDRenderInterface.h"
#include "spkFile.h"
#include "sbaoFile.h"
#include "cseqFile.h"
#include "materialFile.h"
#include <unordered_map>

const ddVec3 red = { 1.0f, 0.0f, 0.0f };
const ddVec3 blue = { 0.0f, 0.0f, 1.0f };
const ddVec3 cyan = { 0.0f, 1.0f, 1.0f };
const ddVec3 magenta = { 1.0f, 0.2f, 0.8f };
const ddVec3 yellow = { 1.0f, 1.0f, 0.0f };
const ddVec3 orange = { 1.0f, 0.5f, 0.0f };
const ddVec3 white = { 1.0f, 1.0f, 1.0f };
const ddVec3 black = { 0.f, 0.f, 0.f };
const ddVec3 green = { 0.0f, 0.6f, 0.0f };

struct BuildingEntity {
	std::string wlu;
	std::string CBatchPath;
	vec3 pos;
	vec3 min, max;
};
std::vector<BuildingEntity> buildingEntities;
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
	SDL_Init(SDL_INIT_EVERYTHING);

	//Setup INI
	std::string wludir;
	float textDrawDistance = 5.f;
	{
		ini_t* ini = ini_load(loadFile("settings.ini").c_str(), NULL);

		int settings_section = ini_find_section(ini, "settings", 0);
		wludir = ini_property_value(ini, settings_section, ini_find_property(ini, settings_section, "wludir", 0));
		textDrawDistance = atof(ini_property_value(ini, settings_section, ini_find_property(ini, settings_section, "textDrawDistance", 0)));

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
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE // flags - see below
	);
	if (window == NULL) {
		printf("Could not create window: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	if (glcontext == NULL) {
		printf("Could not create context: %s\n", SDL_GetError());
		return 1;
	}
	if (SDL_GL_MakeCurrent(window, glcontext) != 0) {
		printf("Could not create context: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GL_SetSwapInterval(1);
	gladLoadGL();
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	
	nk_context *ctx = nk_sdl_init(window);
	nk_font_atlas *atlas;
	nk_sdl_font_stash_begin(&atlas);
	nk_sdl_font_stash_end();
	//Style
	struct nk_color table[NK_COLOR_COUNT];
	table[NK_COLOR_TEXT] = nk_rgba(70, 70, 70, 255);
	table[NK_COLOR_WINDOW] = nk_rgba(175, 175, 175, 255);
	table[NK_COLOR_HEADER] = nk_rgba(175, 175, 175, 255);
	table[NK_COLOR_BORDER] = nk_rgba(0, 0, 0, 255);
	table[NK_COLOR_BUTTON] = nk_rgba(185, 185, 185, 255);
	table[NK_COLOR_BUTTON_HOVER] = nk_rgba(170, 170, 170, 255);
	table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(160, 160, 160, 255);
	table[NK_COLOR_TOGGLE] = nk_rgba(150, 150, 150, 255);
	table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(120, 120, 120, 255);
	table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(175, 175, 175, 255);
	table[NK_COLOR_SELECT] = nk_rgba(190, 190, 190, 255);
	table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(175, 175, 175, 255);
	table[NK_COLOR_SLIDER] = nk_rgba(190, 190, 190, 255);
	table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(80, 80, 80, 255);
	table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(70, 70, 70, 255);
	table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(60, 60, 60, 255);
	table[NK_COLOR_PROPERTY] = nk_rgba(175, 175, 175, 255);
	table[NK_COLOR_EDIT] = nk_rgba(150, 150, 150, 255);
	table[NK_COLOR_EDIT_CURSOR] = nk_rgba(0, 0, 0, 255);
	table[NK_COLOR_COMBO] = nk_rgba(175, 175, 175, 255);
	table[NK_COLOR_CHART] = nk_rgba(160, 160, 160, 255);
	table[NK_COLOR_CHART_COLOR] = nk_rgba(45, 45, 45, 255);
	table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
	table[NK_COLOR_SCROLLBAR] = nk_rgba(180, 180, 180, 255);
	table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(140, 140, 140, 255);
	table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(150, 150, 150, 255);
	table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
	table[NK_COLOR_TAB_HEADER] = nk_rgba(180, 180, 180, 255);
	nk_style_from_table(ctx, table);

	Camera camera;
	camera.type = Camera::FLYCAM;
	camera.far_plane = 4500.f;

	RenderInterface renderInterface;
	dd::initialize(&renderInterface);

	//const std::string wd = "C:/Program Files/Ubisoft/WATCH_DOGS/data_win64/";
	//DatFat df;
	//df.addFat(wd + "common.fat");
	//return 0;
	//df.addFat(wd + "worlds/windy_city/windy_city.fat");

	//df.openRead("domino/user/windycity/main_missions/act_02/mission_09c/a02_m09c.a02_m09c_activation.debug.lua");

	//DominoBox db("D:\\Desktop\\bin\\dlc_solo\\domino\\user\\windycity\\tests\\ai_carfleeing_patterns\\ai_carfleeing_patterns.main.lua");

	tfDIR dir;
	tfDirOpen(&dir, (wludir + std::string("/worlds/windy_city/generated/wlu")).c_str());
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir && strcmp(file.ext, "xml.data.fcb") == 0) {
			printf("Loading %s\n", file.name);

			if (!wlus[file.name].open(file.path)) {
				wlus.erase(file.name);
			}
		}

		tfDirNext(&dir);
	}
	tfDirClose(&dir);

	//Scan Materials
	/*tfDirOpen(&dir, "D:/Desktop/bin/windy_city/graphics/_materials");
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir) {
			printf("Loading %s\n", file.name);

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
			printf("Loading %s\n", file.name);

			sbaoFile spk;
			spk.open(file.path);
		}

		tfDirNext(&dir);
	}
	tfDirClose(&dir);

	//Scan CSeq
	/*tfDirOpen(&dir, (wd + std::string("sequences")).c_str());
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir && strcmp(file.ext, "cseq") == 0) {
			printf("Loading %s\n", file.name);

			cseqFile cseq;
			cseq.open(file.path);

			FILE *fp = fopen("cseq.xml", "w");
			tinyxml2::XMLPrinter printer(fp);
			cseq.root.serializeXML(printer);
			fclose(fp);
		}

		tfDirNext(&dir);
	}
	tfDirClose(&dir);*/

	Uint32 ticks = SDL_GetTicks();
	wluFile &wlu = wlus.begin()->second;
	wlu.shortName = wlus.begin()->first;

	bool windowOpen = true;
	while (windowOpen) {
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
 		camera.update(delta);
		mat4 view = MATlookAt(camera.location, camera.lookingAt, camera.up);
		mat4 projection = MATperspective(camera.fov, (float)windowSize.x / windowSize.y, camera.near_plane, camera.far_plane);
		mat4 vp = projection * view;
		renderInterface.VP = vp;
		renderInterface.windowSize = windowSize;

		//dd::xzSquareGrid(-50.0f, 50.0f, 0.f, 1.f, white);

		/*ImGui::SetNextWindowPos(ImVec2(5.f, 5.f));
		ImGui::Begin("##Top", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::DragFloat3("##Camera", (float*)&camera.location);
		ImGui::End();*/

		/*if (nk_begin(ctx, "Dare", nk_rect(0, 0, 450, 500), 0)) {
			nk_layout_row_dynamic(ctx, 20, 1);
			if (nk_button_label(ctx, "Convert OGG to SBAO")) {
				const char *src = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "ogg\0*.ogg\0", NULL, NULL);
				const char *dst = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, "sbao\0*.sbao\0", NULL, "00000000");
			}
		}
		nk_end(ctx);

		if (nk_begin(ctx, "Scripts", nk_rect(0, 0, 450, 500), 0)) {
			nk_layout_row_dynamic(ctx, 20, 1);
		}
		nk_end(ctx);*/

		//Draw Layer Window
		if (nk_begin(ctx, "Layers", nk_rect(0, 0, 450, windowSize.y), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {

			static char searchWluBuffer[255] = { 0 };
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, searchWluBuffer, sizeof(searchWluBuffer), nk_filter_default);

			nk_layout_row_dynamic(ctx, 100, 1);
			if (nk_group_begin(ctx, "WLUs", NK_WINDOW_BORDER)) {
				nk_layout_row_dynamic(ctx, 10, 1);
				for (auto it = wlus.begin(); it != wlus.end(); ++it) {
					if (nk_strmatch_fuzzy_string(it->first.c_str(), searchWluBuffer, NULL)) {
						int selected = wlu.origFilename == it->second.origFilename;
						if (nk_selectable_label(ctx, it->first.c_str(), NK_TEXT_LEFT, &selected)) {
							wlu = it->second;
							wlu.shortName = it->first;
						}
					}
				}
				nk_group_end(ctx);
			}

			nk_layout_row_dynamic(ctx, 20, 5);
			if (nk_button_label(ctx, "Save")) {
				std::string backup = wlu.origFilename;
				backup += ".bak";
				CopyFileA(wlu.origFilename.c_str(), backup.c_str(), TRUE);

				FILE *fp = fopen(wlu.origFilename.c_str(), "wb");
				wlu.serialize(fp);
				fclose(fp);
			}
			if (nk_button_label(ctx, "Reload")) {
				assert(wlu.open(wlu.origFilename.c_str()));
			}
			if (nk_button_label(ctx, "Restore")) {
				std::string backup = wlu.origFilename;
				backup += ".bak";
				CopyFileA(backup.c_str(), wlu.origFilename.c_str(), FALSE);
				wlu.open(wlu.origFilename.c_str());
			}
			std::string xmlFileName = wlu.shortName + ".xml";
			if (nk_button_label(ctx, "XML")) {
				FILE *fp = fopen(xmlFileName.c_str(), "wb");
				tinyxml2::XMLPrinter printer(fp);
				wlu.root.serializeXML(printer);
				fclose(fp);
			}
			if (nk_button_label(ctx, "Import XML")) {
				tinyxml2::XMLDocument doc;
				doc.LoadFile(xmlFileName.c_str());
				wlu.root.deserializeXML(doc.RootElement());
			}
			nk_layout_row_dynamic(ctx, 20, 1);

			Node *Entities = wlu.root.findFirstChild("Entities");
			if (!Entities) continue;

			for (auto &entity : Entities->children) {
				bool needsCross = true;

				char imguiHash[18];
				snprintf(imguiHash, sizeof(imguiHash), "%p", &entity);

				Attribute *hidName = entity.getAttribute("hidName");
				assert(hidName);

				Attribute *disEntityId = entity.getAttribute("disEntityId");
				assert(disEntityId);
				char disEntityIdS[26];
				snprintf(disEntityIdS, sizeof(disEntityIdS), "%llu", *(uint64_t*)disEntityId->buffer.data());

				Attribute *hidPos = entity.getAttribute("hidPos");
				if (!hidPos) continue;
				Attribute *hidPos_precise = entity.getAttribute("hidPos_precise");
				assert(hidPos_precise);

				vec3 pos = swapYZ(*(vec3*)hidPos->buffer.data());

				if (nk_button_label(ctx, (char*)hidName->buffer.data())) {
					camera.phi = 2.43159294f;
					camera.theta = 3.36464548f;
					camera.location = pos + vec3(1.f, 1.f, 0.f);
				}
				if (nk_button_label(ctx, disEntityIdS)) {
					SDL_SetClipboardText(disEntityIdS);
				}

				//ImGui::DragFloat3((std::string("hidPos##") + imguiHash).c_str(), (float*)hidPos->buffer.data());

				Node *hidBBox = entity.findFirstChild("hidBBox");

				Node *Components = entity.findFirstChild("Components");
				assert(Components);

				Node* CGraphicComponent = Components->findFirstChild("CGraphicComponent");
				if (CGraphicComponent) {
					Attribute* XBG = CGraphicComponent->getAttribute(0x3182766C);

					if (XBG && XBG->buffer.size() > 5) {
						nk_label(ctx, (const char*)XBG->buffer.data(), NK_TEXT_LEFT);
						auto &model = loadXBG((char*)XBG->buffer.data());
						renderInterface.model.use();
						mat4 MVP = vp * MATtranslate(mat4(), pos);
						glUniformMatrix4fv(renderInterface.model.uniforms["MVP"], 1, GL_FALSE, &MVP[0][0]);
						model.draw();
					}
				}

				Attribute *ArchetypeGuid = entity.getAttribute("ArchetypeGuid");
				if (ArchetypeGuid) {
					bool selected;
					if (nk_button_label(ctx, (char*)ArchetypeGuid->buffer.data())) {
						SDL_SetClipboardText((char*)ArchetypeGuid->buffer.data());
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

				//.batch is source files for .cbatch
				/*Attribute* ExportPath = entity.getAttribute("ExportPath");
				if (ExportPath) {
					ImGui::Text("%s", (char*)ExportPath->buffer.data());
				}*/

				if (pos.distance(camera.location) < textDrawDistance)
					dd::projectedText((char*)hidName->buffer.data(), &pos.x, white, &vp[0][0], 0, 0, windowSize.x, windowSize.y, 0.5f);
				if (needsCross)
					dd::cross(&pos.x, 0.25f);
			}
		}
		nk_end(ctx);

		//Render Buildings
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

		glBindVertexArray(VertexArrayID);
		dd::flush(0);
		nk_sdl_render(NK_ANTI_ALIASING_ON, 4 * 1024 * 1024, 2 * 1024 * 1024);
		SDL_GL_SwapWindow(window);

		nk_input_begin(ctx);
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
						windowOpen = false;
						exit(0);
					}
					break;
			}

			nk_sdl_handle_event(&event);
		}
		nk_input_end(ctx);
	}

	return 0;
}