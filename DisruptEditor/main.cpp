#include "glad.h"
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "ImGuizmo.h"
#define DEBUG_DRAW_IMPLEMENTATION
#include "debug_draw.hpp"
#include <stdio.h>
#include <string>
#include <algorithm>
#include <cctype>

#include "wluFile.h"
#include "xbgFile.h"
#include "DatFat.h"
#define TINYFILES_IMPL
#include "tinyfiles.h"

#include "Camera.h"

#include "GLHelper.h"
#include <Shlwapi.h>

const static char* wd = "D:/Desktop/bin/windy_city_unpack/";

uint64_t fileHash(std::string file) {
	std::transform(file.begin(), file.end(), file.begin(), ::tolower);

	uint64_t hash = 0xCBF29CE484222325;
	for (auto it = file.begin(); it != file.end(); ++it) {
		hash *= 0x100000001B3;
		hash ^= *it;
	}

	return hash;
}

std::string getPathToFile(const char* file) {
	std::string realPath = wd;
	realPath.append(file);
	if(PathFileExistsA(realPath.c_str()))
		return realPath;

	uint32_t hash = fileHash(file);
	char buffer[500];
	snprintf(buffer, sizeof(buffer), "__UNKNOWN\\gfx\\%08X.xbg", hash);

	return wd + std::string(buffer);
}

vec3 swapYZ(const vec3 &ref) {
	return vec3(ref.x, ref.z, ref.y);
}

class RenderInterface : public dd::RenderInterface {
public:
	RenderInterface();
	//
	// These are called by dd::flush() before any drawing and after drawing is finished.
	// User can override these to perform any common setup for subsequent draws and to
	// cleanup afterwards. By default, no-ops stubs are provided.
	//
	void beginDraw() {}
	void endDraw() {}

	//
	// Create/free the glyph bitmap texture used by the debug text drawing functions.
	// The debug renderer currently only creates one of those on startup.
	//
	// You're not required to implement these two if you don't care about debug text drawing.
	// Default no-op stubs are provided by default, which disable debug text rendering.
	//
	// Texture dimensions are in pixels, data format is always 8-bits per pixel (Grayscale/GL_RED).
	// The pixel values range from 255 for a pixel within a glyph to 0 for a transparent pixel.
	// If createGlyphTexture() returns null, the renderer will disable all text drawing functions.
	//
	dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void * pixels);
	void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex);

	//
	// Batch drawing methods for the primitives used by the debug renderer.
	// If you don't wish to support a given primitive type, don't override the method.
	//
	void drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled);
	void drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled);
	void drawGlyphList(const dd::DrawVertex * glyphs, int count, dd::GlyphTextureHandle glyphTex);

	Shader lines, tex, model;
	VertexBuffer linesBuffer, texBuffer;

	mat4 VP;
	ivec2 windowSize;
};

RenderInterface::RenderInterface() {
	lines = loadShaders("gldd.xml");
	linesBuffer = createVertexBuffer(NULL, 0, BUFFER_STREAM);
	glEnable(GL_PROGRAM_POINT_SIZE);

	tex = loadShaders("tex.xml");
	texBuffer = createVertexBuffer(NULL, 0, BUFFER_STREAM);

	model = loadShaders("model.xml");
}

void RenderInterface::drawPointList(const dd::DrawVertex *points, int count, bool depthEnabled) {
	updateVertexBuffer(points, count * sizeof(dd::DrawVertex), linesBuffer);

	lines.use();
	glUniformMatrix4fv(lines.uniforms["MVP"], 1, GL_FALSE, &VP[0][0]);
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, linesBuffer.buffer_id);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(float) * 7,  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		sizeof(float) * 7,                // stride
		(GLvoid*)(3 * sizeof(GLfloat))    // array buffer offset
	);

	// Draw the triangles
	glDrawArrays(GL_POINTS, 0, count);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void RenderInterface::drawLineList(const dd::DrawVertex *points, int count, bool depthEnabled) {
	updateVertexBuffer(points, count * sizeof(dd::DrawVertex), linesBuffer);

	lines.use();
	glUniformMatrix4fv(lines.uniforms["MVP"], 1, GL_FALSE, &VP[0][0]);
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, linesBuffer.buffer_id);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(float) * 7,  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		sizeof(float) * 7,                // stride
		(GLvoid*)(3 * sizeof(GLfloat))    // array buffer offset
	);

	// Draw the triangles
	glDrawArrays(GL_LINES, 0, count);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void RenderInterface::drawGlyphList(const dd::DrawVertex * glyphs, int count, dd::GlyphTextureHandle glyphTex) {
	updateVertexBuffer(glyphs, count * sizeof(dd::DrawVertex), texBuffer);

	tex.use();
	glUniform2f(tex.uniforms["windowSize"], windowSize.x, windowSize.y);
	glBindBuffer(GL_ARRAY_BUFFER, texBuffer.buffer_id);
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(float) * 7,  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		sizeof(float) * 7,                // stride
		(GLvoid*)(2 * sizeof(GLfloat))    // array buffer offset
	);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		3,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		sizeof(float) * 7,                // stride
		(GLvoid*)(4 * sizeof(GLfloat))    // array buffer offset
	);

	// Draw the triangles
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, (GLuint)glyphTex);
	glDrawArrays(GL_TRIANGLES, 0, count);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

dd::GlyphTextureHandle RenderInterface::createGlyphTexture(int width, int height, const void * pixels) {
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return (dd::GlyphTextureHandle)textureId;
}

void RenderInterface::destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) {
	glDeleteTextures(1, (GLuint*)&glyphTex);
}

const ddVec3 red = { 1.0f, 0.0f, 0.0f };
const ddVec3 blue = { 0.0f, 0.0f, 1.0f };
const ddVec3 cyan = { 0.0f, 1.0f, 1.0f };
const ddVec3 magenta = { 1.0f, 0.2f, 0.8f };
const ddVec3 yellow = { 1.0f, 1.0f, 0.0f };
const ddVec3 orange = { 1.0f, 0.5f, 0.0f };
const ddVec3 white = { 1.0f, 1.0f, 1.0f };
const ddVec3 green = { 0.0f, 0.6f, 0.0f };

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
	ImGui_ImplSdlGL3_Init(window);

	Camera camera;
	camera.type = Camera::FLYCAM;
	camera.far_plane = 4096.f;

	RenderInterface renderInterface;
	dd::initialize(&renderInterface);

	std::map<std::string, wluFile> wlus;
	std::map<std::string, xbgFile> xbgs;

	tfDIR dir;
	tfDirOpen(&dir, (wd + std::string("worlds/windy_city/generated/wlu")).c_str());
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

	//const std::string wd = "C:/Program Files/Ubisoft/WATCH_DOGS/data_win64/";
	//DatFat df;
	//df.addFat(wd + "common.fat");
	//df.addFat(wd + "worlds/windy_city/windy_city.fat");

	//df.openRead("worlds/windy_city/generated/wlu/wlu_data_near233.xml.data.fcb");

	Uint32 ticks = SDL_GetTicks();

	bool windowOpen = true;
	while (windowOpen) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ImGui_ImplSdlGL3_NewFrame(window);
		ImGuizmo::BeginFrame();

		float delta = (SDL_GetTicks() - ticks) / 1000.f;
		ticks = SDL_GetTicks();

		ivec2 windowSize;
		SDL_GetWindowSize(window, &windowSize.x, &windowSize.y);
		glViewport(0, 0, windowSize.x, windowSize.y);
		camera.update(delta);
		mat4 view = MATlookAt(camera.location, camera.lookingAt, camera.up);
		mat4 projection = MATperspective(camera.fov, (float)windowSize.x / windowSize.y, camera.near_plane, camera.far_plane);
		mat4 vp = projection * view;
		renderInterface.VP = vp;
		renderInterface.windowSize = windowSize;

		dd::xzSquareGrid(-50.0f, 50.0f, 0.f, 1.f, white);

		ImGui::SetNextWindowPos(ImVec2(5.f, 5.f));
		ImGui::Begin("##Top", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::DragFloat3("##Camera", (float*)&camera.location);
		ImGui::End();

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

			if (ImGui::Button("XML")) {
				FILE *fp = fopen("test.xml", "wb");
				tinyxml2::XMLPrinter printer(fp);
				wlu.root.serializeXML(printer);
				fclose(fp);
			}

			Node *Entities = wlu.root.findFirstChild("Entities");
			assert(Entities);

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
				assert(hidPos);
				Attribute *hidPos_precise = entity.getAttribute("hidPos_precise");
				assert(hidPos_precise);

				vec3 pos = swapYZ(*(vec3*)hidPos->buffer.data());

				ImGui::Separator();
				ImGui::Text("%s", hidName->buffer.data());
				if (ImGui::Selectable(disEntityIdS)) {
					SDL_SetClipboardText(disEntityIdS);
				}
				
				ImGui::DragFloat3((std::string("hidPos##") + imguiHash).c_str(), (float*)hidPos->buffer.data());
				
				Node *hidBBox = entity.findFirstChild("hidBBox");

				Node *Components = entity.findFirstChild("Components");
				assert(Components);

				Node* CGraphicComponent = Components->findFirstChild("CGraphicComponent");
				if (CGraphicComponent) {
					Attribute* XBG = CGraphicComponent->getAttribute(0x3182766C);

					if (XBG && XBG->buffer.size() > 5) {
						ImGui::Text("%s", XBG->buffer.data());
						if (xbgs.count((char*)XBG->buffer.data()) == 0) {
							auto &model = xbgs[(char*)XBG->buffer.data()];
							printf("Loading %s...\n", XBG->buffer.data());
							model.open(getPathToFile((char*)(XBG->buffer.data())).c_str());
						}

						auto &model = xbgs[(char*)XBG->buffer.data()];
						renderInterface.model.use();
						mat4 MVP = vp * MATtranslate(mat4(), pos);
						glUniformMatrix4fv(renderInterface.model.uniforms["MVP"], 1, GL_FALSE, &MVP[0][0]);
						model.draw();
					}
				}

				Attribute *ArchetypeGuid = entity.getAttribute("ArchetypeGuid");
				if (ArchetypeGuid) {
					bool selected;
					if (ImGui::Selectable((char*)ArchetypeGuid->buffer.data())) {
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

				if (pos.distance(camera.location) < 5.f)
					dd::projectedText((char*)hidName->buffer.data(), &pos.x, white, &vp[0][0], 0, 0, windowSize.x, windowSize.y, 0.5f);
				if(needsCross)
					dd::cross(&pos.x, 0.25f);
			}
		}
		ImGui::End();

		dd::flush(0);
		ImGui::Render();

		SDL_GL_SwapWindow(window);
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_CLOSE)
						windowOpen = false;
					break;
			}

			ImGui_ImplSdlGL3_ProcessEvent(&event);
		}
	}

	return 0;
}