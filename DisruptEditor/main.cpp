#include "glad.h"
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include <stdio.h>
#include <string>

#include "wluFile.h"
#include "DatFat.h"
#define TINYFILES_IMPL
#include "tinyfiles.h"

int main(int argc, char **argv) {
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
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
	ImGui_ImplSdlGL3_Init(window);

	std::string wd = "D:/Desktop/bin/windy_city_unpack/worlds/windy_city/generated/wlu";
	std::map<std::string, wluFile> wlus;

	tfDIR dir;
	tfDirOpen(&dir, wd.c_str());

	int count = 0;

	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir && strcmp(file.ext, "xml.data.fcb") == 0) {
			printf("Loading %s\n", file.name);

			wlus[file.name].open(file.path);
		}

		tfDirNext(&dir);

		count++;

		if (count > 25)
			break;
	}

	tfDirClose(&dir);

	//const std::string wd = "C:/Program Files/Ubisoft/WATCH_DOGS/data_win64/";
	//DatFat df;
	//df.addFat(wd + "common.fat");
	//df.addFat(wd + "worlds/windy_city/windy_city.fat");

	//df.openRead("worlds/windy_city/generated/wlu/wlu_data_near233.xml.data.fcb");

	bool windowOpen = true;
	while (windowOpen) {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplSdlGL3_NewFrame(window);

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New World")) {}
				if (ImGui::MenuItem("Open World", "Ctrl+O")) {}
				if (ImGui::MenuItem("Save", "Ctrl+S")) {}
				if (ImGui::MenuItem("Save As..")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Quit", "Alt+F4")) {
					exit(0);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit")) {
				if (ImGui::MenuItem("Undo", "CTRL+Z", false, false)) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X", false, false)) {}
				if (ImGui::MenuItem("Copy", "CTRL+C", false, false)) {}
				if (ImGui::MenuItem("Paste", "CTRL+V", false, false)) {}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}


		//Draw Layer Window
		ImGui::Begin("Layers");

		static int cur = -1;
		std::vector<const char*> items;
		for (auto it = wlus.begin(); it != wlus.end(); ++it)
			items.push_back(it->first.c_str());
		ImGui::ListBox("##WLU", &cur, items.data(), items.size());

		if (cur > -1 && cur < wlus.size()) {
			ImGui::Text("%s", items[cur]);

			wluFile &wlu = wlus[items[cur]];

			Node *Entities = wlu.root.findFirstChild("Entities");
			assert(Entities);

			for (auto &entity : Entities->children) {
				Attribute *hidName = entity.getAttribute("hidName");
				assert("hidName");

				Attribute *hidPos = entity.getAttribute("hidPos");
				assert("hidPos");

				ImGui::Separator();
				ImGui::Text("%s", hidName->buffer.data());
				ImGui::InputFloat3("hidPos", (float*)hidPos->buffer.data());
				
				Node *Components = entity.findFirstChild("Components");
				assert(Components);

				Node* CGraphicComponent = Components->findFirstChild("CGraphicComponent");
				if (CGraphicComponent) {
					Attribute* XBG = CGraphicComponent->getAttribute(0x3182766C);
					if(XBG)
						ImGui::Text("%s", XBG->buffer.data());
				}

			}
		}

		ImGui::End();

		ImGui::Render();

		SDL_GL_SwapWindow(window);
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSdlGL3_ProcessEvent(&event);
		}
	}

	return 0;
}