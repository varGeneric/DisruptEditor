#include "xbgFile.h"

#include <iostream>
#include "Vector.h"
#include <stdlib.h>
#include "Common.h"
#include "materialFile.h"
#include "xbtFile.h"
#include "glm/glm.hpp"
#include <SDL_log.h>
#include <SDL_rwops.h>
#include <SDL_messagebox.h>

#pragma pack(push, 1)
struct XBGHead {
	uint32_t magic;
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint32_t unk[26];
	uint32_t lodCount;
};
struct Bone {
	int32_t unknown;
	float pos[3];
	float rot[4];
	int16_t parent;
	int16_t id;
	uint32_t hash;
};
struct Unknown {
	int32_t u1[2];
	float u2[7];
	int32_t u3[5];
	int32_t type;
};
struct MeshData {
	float u1[10];
	uint16_t u2[20];
	uint32_t matCount;
	uint32_t u3[2];
};
#pragma pack(pop)

static inline void seekpad(SDL_RWops *fp, long pad) {
	//16-byte chunk alignment
	long size = SDL_RWtell(fp);
	long seek = (pad - (size % pad)) % pad;
	SDL_RWseek(fp, seek, RW_SEEK_CUR);
}

static inline std::string readString(SDL_RWops *fp) {
	uint32_t size = SDL_ReadLE32(fp);
	std::string str(size, '\0');
	SDL_RWread(fp, &str[0], 1, size);
	return str;
}

void xbgFile::open(const char *file) {
	SDL_RWops *fp = SDL_RWFromFile(file, "rb");
	if (!fp) {
		SDL_Log("Failed\n");
		return;
	}
	Vector<uint8_t> data(SDL_RWsize(fp));
	SDL_RWread(fp, data.data(), data.size(), 1);
	SDL_RWclose(fp);
	fp = SDL_RWFromConstMem(data.data(), data.size());

	XBGHead head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == 1195724621);
	SDL_assert_release(head.majorVersion == 97);
	SDL_assert_release(head.minorVersion == 50);

	SDL_RWseek(fp, head.lodCount * sizeof(float), RW_SEEK_CUR);
	SDL_RWseek(fp, 8, RW_SEEK_CUR);
	uint32_t a = SDL_ReadLE32(fp);
	SDL_RWseek(fp, (head.lodCount - a) * sizeof(float), RW_SEEK_CUR);

	//Section A - Materials
	{
		uint32_t count = SDL_ReadLE32(fp);
		for (uint32_t i = 0; i < count; ++i) {
			Material mat;
			mat.hash = SDL_ReadLE32(fp);
			mat.file = readString(fp);
			seekpad(fp, 4);
			materials.push_back(mat);
		}
	}

	//Section B -
	{
		uint32_t matCount = SDL_ReadLE32(fp);
		for (uint32_t i = 0; i < matCount; ++i) {
			uint32_t hash = SDL_ReadLE32(fp);
			std::string matFile = readString(fp);
			seekpad(fp, 4);
			SDL_ReadLE32(fp);
		}

		//Bl
		uint32_t count = SDL_ReadLE32(fp);
		for (uint32_t i = 0; i < count; ++i) {
			uint32_t hash = SDL_ReadLE32(fp);
			std::string matFile = readString(fp);
			seekpad(fp, 4);
		}
	}

	//Section C -
	{
		uint32_t boneMapCount = SDL_ReadLE32(fp);
		for (uint32_t i = 0; i < boneMapCount; ++i) {
			uint32_t boneIDCount = SDL_ReadLE32(fp);
			uint16_t boneMap = SDL_ReadLE16(fp);
			seekpad(fp, 4);
		}
	}

	//Section D -
	{
		uint32_t boneChunk = SDL_ReadLE32(fp);
		if (boneChunk == 1) {
			uint32_t boneCount = SDL_ReadLE32(fp);
			for (uint32_t i = 0; i < boneCount; ++i) {
				Bone bone;
				SDL_RWread(fp, &bone, sizeof(bone), 1);
				std::string name = readString(fp);
				seekpad(fp, 4);
			}
		}
	}

	//Section E -
	{
		uint32_t f1 = SDL_ReadLE32(fp);
		uint32_t matrixCount = SDL_ReadLE32(fp);
		seekpad(fp, 16);

		Vector<glm::mat4> data(matrixCount);
		SDL_RWread(fp, data.data(), sizeof(glm::mat4), data.size());
	}

	//Section K -
	{
		uint32_t f1 = SDL_ReadLE32(fp);
		if (f1 > 0) {
			uint32_t size = SDL_ReadLE32(fp);
			SDL_RWseek(fp, size, RW_SEEK_CUR);
		}
	}

	//Section G - TODO
	{
		uint32_t count = SDL_ReadLE32(fp);
		if (count != 0)
			return;
		for (int32_t i = 0; i < count; ++i) {
			Unknown u;
			SDL_RWread(fp, &u, sizeof(u), 1);

			/*if (u.type == 2) {
				count=g.i(1)[0]
				count=g.i(1)[0]
				for m in range(count):
					hash=g.i(1)[0]
					m,g.word(g.i(1)[0])
					g.seekpad(16)
					g.f(4)	
					g.f(4)	
					g.f(4)	
					g.f(4)	
					g.f(1)
				g.i(1)[0]
				count=g.i(1)[0]
				for m in range(count):
					hash=g.i(1)[0]
					m,g.word(g.i(1)[0])
					g.seekpad(16)
					g.f(4)	
					g.f(4)	
					g.f(4)	
					g.f(4)	
					g.f(7)
				count=g.i(1)[0]
				for m in range(count):
					hash=g.i(1)[0]
					m,g.word(g.i(1)[0])
					g.seekpad(16)
					g.f(4)	
					g.f(4)	
					g.f(4)	
					g.f(4)	
					g.f(6)
				count=g.i(1)[0]	
				for m in range(count):
					cv=g.i(1)[0]
					g.tell()
					count=g.i(1)[0]
					for m in range(count):
						g.f(5)
				count=g.i(1)[0]		
				for m in range(count):
					cv=g.i(1)[0]
					count=g.i(1)[0]	
					for m in range(count):
						g.f(9)
				count=g.i(1)[0]		
				for m in range(count):
					cv=g.i(1)[0]
					count=g.i(1)[0]	
					for m in range(count):
						g.f(9)
				count=g.i(1)[0]
				for m in range(count):
					hash=g.i(1)[0]
					m,g.word(g.i(1)[0])
					g.seekpad(4)
					g.f(4)
				
				count=g.i(1)[0]
				for m in range(count):
					hash=g.i(1)[0]
					m,g.word(g.i(1)[0])
					g.seekpad(4)
					
				count=g.i(1)[0]
				count=g.i(1)[0]
				count=g.i(1)[0]
				count=g.i(1)[0]
			} else {

			}*/

		}
	}

	//Section F - TODO
	{
		uint32_t count = SDL_ReadLE32(fp);
		SDL_assert_release(count == 0);
		for (int32_t i = 0; i < count; ++i) {

		}
	}

	//Parse Mesh
	for (int32_t i = 0; i < head.lodCount; ++i) {
		uint32_t meshCount = SDL_ReadLE32(fp);
		try {
			meshes.resize(meshCount);
		}
		catch (std::bad_alloc& e)
		{
			char *message = new char[74 + strlen(file)];
			sprintf(message, "malloc() failed on file: %s, consider removing this file and trying again. ", file);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "malloc Error", message, NULL);
			delete message;

			exit(1);
		}
		for (uint32_t j = 0; j < meshCount; ++j) {
			Mesh &mesh = meshes[j];

			MeshData data;
			SDL_RWread(fp, &data, sizeof(data), 1);

			mesh.vertexStride = data.u2[4];
			mesh.matID = data.u2[2];
			mesh.vertexCount = 1 + data.u2[18] - data.u2[17];
			mesh.totalVertexCount = data.u2[16];
			mesh.faceCount = data.u2[10];
			mesh.UVFlag = data.u2[3];
			mesh.scaleFlag = 0;
			mesh.boneMapID = data.u2[6];

			for (int32_t k = 0; k < data.matCount; ++k) {
				SDL_RWseek(fp, 68, RW_SEEK_CUR);
				mesh.mat = readString(fp);
				seekpad(fp, 4);

				uint16_t u[2];
				SDL_RWread(fp, u, sizeof(uint16_t), 2);
			}
		}
	}

	uint32_t min = SDL_ReadLE32(fp);
	uint32_t max = SDL_ReadLE32(fp);

	struct Model {

	};
	Vector<Model> models(head.lodCount);

	for (uint32_t i = 0; i < max; ++i) {
		uint32_t meshCount = SDL_ReadLE32(fp);

		for (int32_t j = 0; j < meshes.size(); ++j) {
			Mesh &mesh = meshes[j];

			mesh.buffer.resize(mesh.vertexCount * mesh.vertexStride / sizeof(uint16_t));
			SDL_RWread(fp, mesh.buffer.data(), sizeof(uint16_t), mesh.buffer.size());
			mesh.vbo = createVertexBuffer(mesh.buffer.data(), sizeof(uint16_t) * mesh.buffer.size(), BUFFER_STATIC);
		}

		//Faces
		SDL_ReadLE32(fp);
		for (int32_t j = 0; j < meshes.size(); ++j) {
			Mesh &mesh = meshes[j];

			mesh.index.resize(mesh.faceCount * 3);
			SDL_RWread(fp, mesh.index.data(), sizeof(uint16_t), mesh.index.size());
			mesh.ibo = createVertexBuffer(mesh.index.data(), sizeof(uint16_t) * mesh.index.size(), BUFFER_STATIC);
		}

		seekpad(fp, 4);

		break;
	}

	SDL_RWclose(fp);
}

void xbgFile::draw() {
	auto material = materials.begin();
	for (auto &mesh : meshes) {
		auto &mat = loadMaterial(material->file);
		if (!mat.entries.empty()) {
			auto &texture = loadTexture(mat.entries.begin()->texture);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture.id);
		}

		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo.buffer_id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo.buffer_id);
		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_SHORT,  // type
			GL_TRUE,           // normalized?
			mesh.vertexStride,  // stride
			(void*)0            // array buffer offset
		);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(
			1,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			2,                  // size
			GL_SHORT,  // type
			GL_TRUE,           // normalized?
			mesh.vertexStride,  // stride
			(void*)(8)            // array buffer offset
		);

		// Draw the triangles
		GLint id;
		glGetIntegerv(GL_CURRENT_PROGRAM, &id);
		glDrawElements(GL_TRIANGLES, mesh.faceCount * 3, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		++material;
	}
}
