#include "glad.h"
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "ImGuizmo.h"
#define DEBUG_DRAW_IMPLEMENTATION
#include "debug_draw.hpp"
#include <stdio.h>
#include <string>

#include "wluFile.h"
#include "DatFat.h"
#define TINYFILES_IMPL
#include "tinyfiles.h"

#include "Camera.h"

vec3 swapYZ(const vec3 &ref) {
	return vec3(ref.x, ref.z, ref.y);
}

class Shader {
public:
	GLuint oglid;
	void use() { glUseProgram(oglid); }
	std::map<std::string, GLint> uniforms;
	enum UniformTypes { INT, FLOAT, MAT4, VEC2, VEC3, VEC4, TEXTURE2D, TEXTURE3D };
	std::map<std::string, std::string> uniformTypes;
	std::vector<std::string> samplerAttributes;
};

bool loadShader(Shader *shader, const char * vertex, const char * fragment) {
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	glShaderSource(VertexShaderID, 1, &vertex, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	if (Result == GL_FALSE) {
		glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		return false;
	}

	// Compile Fragment Shader
	glShaderSource(FragmentShaderID, 1, &fragment, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	if (Result == GL_FALSE) {
		glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		glDeleteShader(VertexShaderID);
		glDeleteShader(FragmentShaderID);
		return false;
	}

	// Link the program
	shader->oglid = glCreateProgram();
	glAttachShader(shader->oglid, VertexShaderID);
	glAttachShader(shader->oglid, FragmentShaderID);

	glLinkProgram(shader->oglid);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	// Check the program
	glGetProgramiv(shader->oglid, GL_LINK_STATUS, &Result);
	if (Result == GL_FALSE) {
		glGetProgramiv(shader->oglid, GL_INFO_LOG_LENGTH, &InfoLogLength);
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(shader->oglid, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		glDeleteProgram(shader->oglid);
		return false;
	}

	glUseProgram(shader->oglid);

	for (auto it = shader->uniforms.begin(), end = shader->uniforms.end(); it != end; ++it) {
		it->second = glGetUniformLocation(shader->oglid, it->first.c_str());
	}

	for (int i = 0; i < shader->samplerAttributes.size(); ++i) {
		glUniform1i(shader->uniforms[shader->samplerAttributes[i]], i);
	}

	return true;
}

Shader loadShaders(const char *program) {
	Shader shader;

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	tinyxml2::XMLDocument doc;
	doc.LoadFile(program);

	tinyxml2::XMLElement *root = doc.RootElement();

	tinyxml2::XMLElement *uniform = root->FirstChildElement("uniform");
	while (uniform) {
		const char* name = uniform->Attribute("name");
		const char* type = uniform->Attribute("type");

		shader.uniforms[name] = -1;
		shader.uniformTypes[name] = type;
		if (type == std::string("sampler2D") || type == std::string("sampler3D")) {
			shader.samplerAttributes.push_back(name);
		}

		uniform = uniform->NextSiblingElement("uniform");
	}

	assert(loadShader(&shader, root->FirstChildElement("vertex")->GetText(), root->FirstChildElement("fragment")->GetText()));

	return shader;
}

enum VertexBufferOptions { BUFFER_STATIC, BUFFER_DYNAMIC, BUFFER_STREAM };

class VertexBuffer {
public:
	bool loaded() { return buffer_id != 0; }
	uint32_t buffer_id = 0;
	unsigned long size = 0, maxsize = 0;
	VertexBufferOptions type;
};

VertexBuffer createVertexBuffer(const void *data, unsigned long size, VertexBufferOptions type) {
	VertexBuffer oglvb;
	glGenBuffers(1, &oglvb.buffer_id);
	glBindBuffer(GL_ARRAY_BUFFER, oglvb.buffer_id);
	oglvb.type = type;
	switch (type) {
		case BUFFER_STATIC:
			glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
			break;

		case BUFFER_DYNAMIC:
			glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
			break;

		case BUFFER_STREAM:
			glBufferData(GL_ARRAY_BUFFER, size, data, GL_STREAM_DRAW);
			break;
	}

	oglvb.maxsize = size;
	oglvb.size = size;

	return oglvb;
}

void updateVertexBuffer(const void *data, unsigned long size, VertexBuffer &buffer) {
	assert(buffer.buffer_id != 0);

	glBindBuffer(GL_ARRAY_BUFFER, buffer.buffer_id);
	if (buffer.maxsize < size) {
		buffer.maxsize = size;
		switch (buffer.type) {
			case BUFFER_STATIC:
				glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
				break;

			case BUFFER_DYNAMIC:
				glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
				break;

			case BUFFER_STREAM:
				glBufferData(GL_ARRAY_BUFFER, size, data, GL_STREAM_DRAW);
				break;
		}
	} else {
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}
	buffer.size = size;
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

	Shader lines, tex;
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

	RenderInterface renderInterface;
	dd::initialize(&renderInterface);

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
		camera.update(delta);
		mat4 view = MATlookAt(camera.location, camera.lookingAt, camera.up);
		mat4 projection = MATperspective(camera.fov, (float)windowSize.x / windowSize.y, camera.near_plane, camera.far_plane);
		mat4 vp = projection * view;
		renderInterface.VP = vp;
		renderInterface.windowSize = windowSize;

		dd::xzSquareGrid(-50.0f, 50.0f, 0.f, 1.f, white);

		ImGui::SetNextWindowPos(ImVec2(5.f, 5.f));
		ImGui::Begin("", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
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

			Node *Entities = wlu.root.findFirstChild("Entities");
			assert(Entities);

			for (auto &entity : Entities->children) {
				char imguiHash[18];
				snprintf(imguiHash, sizeof(imguiHash), "%p", &entity);

				Attribute *hidName = entity.getAttribute("hidName");
				assert(hidName);

				Attribute *hidPos = entity.getAttribute("hidPos");
				assert(hidPos);
				Attribute *hidPos_precise = entity.getAttribute("hidPos_precise");
				assert(hidPos_precise);

				ImGui::Separator();
				ImGui::Text("%s", hidName->buffer.data());
				ImGui::DragFloat3((std::string("hidPos##") + imguiHash).c_str(), (float*)hidPos->buffer.data());
				
				Node *hidBBox = entity.findFirstChild("hidBBox");

				Node *Components = entity.findFirstChild("Components");
				assert(Components);

				Node* CGraphicComponent = Components->findFirstChild("CGraphicComponent");
				if (CGraphicComponent) {
					Attribute* XBG = CGraphicComponent->getAttribute(0x3182766C);
					if(XBG)
						ImGui::Text("%s", XBG->buffer.data());
				}

				vec3 pos = swapYZ(*(vec3*)hidPos->buffer.data());
				if(pos.distance(camera.location) < 5.f)
					dd::projectedText((char*)hidName->buffer.data(), &pos.x, white, &vp[0][0], 0, 0, windowSize.x, windowSize.y, 0.5f);

				dd::cross(&pos.x, 1.f);

				if (hidBBox) {
					vec3 boxMin = swapYZ(*((vec3*)hidBBox->getAttribute("vectorBBoxMin")->buffer.data()));
					vec3 boxMax = swapYZ(*((vec3*)hidBBox->getAttribute("vectorBBoxMax")->buffer.data()));
					vec3 boxExtent = boxMax - boxMin;
					dd::box(&pos.x, blue, boxExtent.x, boxExtent.y, boxExtent.z);
				}
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