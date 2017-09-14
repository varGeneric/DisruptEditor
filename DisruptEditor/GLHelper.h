#pragma once

#include "glad.h"
#include <map>
#include <string>
#include <vector>
#include <assert.h>
#include "tinyxml2.h"

class Shader {
public:
	GLuint oglid;
	void use() { glUseProgram(oglid); }
	std::map<std::string, GLint> uniforms;
	enum UniformTypes { INT, FLOAT, MAT4, VEC2, VEC3, VEC4, TEXTURE2D, TEXTURE3D };
	std::map<std::string, std::string> uniformTypes;
	std::vector<std::string> samplerAttributes;
};

inline bool loadShader(Shader *shader, const char * vertex, const char * fragment) {
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

inline Shader loadShaders(const char *program) {
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

inline VertexBuffer createVertexBuffer(const void *data, unsigned long size, VertexBufferOptions type) {
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

inline void updateVertexBuffer(const void *data, unsigned long size, VertexBuffer &buffer) {
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
