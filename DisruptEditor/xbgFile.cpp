#include "xbgFile.h"

#include <iostream>
#include <vector>
#include <stdlib.h>

xbgFile::xbgFile() {}


xbgFile::~xbgFile() {}

struct XBGHead {
	char magic[4];
	unsigned short unknown[14];
	float unknown2[20];
	int32_t lodCount;
};

static inline void seekpad(FILE *fp, long pad) {
	//16-byte chunk alignment
	long size = ftell(fp);
	long seek = (pad - (size % pad)) % pad;
	fseek(fp, seek, SEEK_CUR);
}

void xbgFile::open(const char *file) {
	FILE* fp = fopen(file, "rb");
	if (!fp) {
		printf("Failed\n");
		return;
	}

	XBGHead head;
	fread(&head, sizeof(head), 1, fp);

	fseek(fp, head.lodCount * sizeof(float), SEEK_CUR);
	fseek(fp, 8, SEEK_CUR);
	int32_t a;
	fread(&a, sizeof(a), 1, fp);
	fseek(fp, (head.lodCount - a) * sizeof(float), SEEK_CUR);

	//Section A - Materials
	{
		int32_t count;
		fread(&count, sizeof(count), 1, fp);
		for (int32_t i = 0; i < count; ++i) {
			char hash[4];
			fread(hash, sizeof(hash), 1, fp);
			int32_t matFileSize;
			fread(&matFileSize, sizeof(matFileSize), 1, fp);
			std::string matFile(matFileSize, '\0');
			fread(&matFile[0], 1, matFileSize, fp);
			seekpad(fp, 4);
		}
	}

	//Section B -
	{
		int32_t matCount;
		fread(&matCount, sizeof(matCount), 1, fp);
		for (int32_t i = 0; i < matCount; ++i) {
			int32_t hash;
			fread(&hash, sizeof(hash), 1, fp);
			int32_t matFileSize;
			fread(&matFileSize, sizeof(matFileSize), 1, fp);
			std::string matFile(matFileSize, '\0');
			fread(&matFile[0], 1, matFileSize, fp);
			seekpad(fp, 4);
			fread(&a, sizeof(a), 1, fp);
		}

		//Bl
		int32_t count;
		fread(&count, sizeof(count), 1, fp);
		for (int32_t i = 0; i < count; ++i) {
			int32_t hash;
			fread(&hash, sizeof(hash), 1, fp);
			int32_t matFileSize;
			fread(&matFileSize, sizeof(matFileSize), 1, fp);
			std::string matFile(matFileSize, '\0');
			fread(&matFile[0], 1, matFileSize, fp);
			seekpad(fp, 4);
		}
	}

	//Section C -
	{
		int32_t boneMapCount;
		fread(&boneMapCount, sizeof(boneMapCount), 1, fp);
		for (int32_t i = 0; i < boneMapCount; ++i) {
			int32_t boneIDCount;
			fread(&boneIDCount, sizeof(boneIDCount), 1, fp);
			int16_t boneMap;
			fread(&boneMap, sizeof(boneMap), 1, fp);
			seekpad(fp, 4);
		}
	}

	//Section D -
	{
		struct Bone {
			int32_t unknown;
			float pos[3];
			float rot[4];
			int16_t parent;
			int16_t id;
			uint32_t hash;
		};

		int32_t boneChunk;
		fread(&boneChunk, sizeof(boneChunk), 1, fp);
		if (boneChunk == 1) {
			int32_t boneCount;
			fread(&boneCount, sizeof(boneCount), 1, fp);
			for (int32_t i = 0; i < boneCount; ++i) {
				Bone bone;
				fread(&bone, sizeof(bone), 1, fp);

				int32_t nameSize;
				fread(&nameSize, sizeof(nameSize), 1, fp);
				std::string name(nameSize, '\0');
				fread(&name[0], 1, nameSize, fp);
				seekpad(fp, 4);
			}
		}
	}

	//Section E -
	{
		int32_t f1, matrixCount;
		fread(&f1, sizeof(f1), 1, fp);
		fread(&matrixCount, sizeof(matrixCount), 1, fp);
		seekpad(fp, 16);

		for (int32_t i = 0; i < matrixCount; ++i) {
			float mat[16];
			fread(mat, sizeof(float), 16, fp);
		}
	}

	//Section K -
	{
		int32_t f1;
		fread(&f1, sizeof(f1), 1, fp);
		if (f1 > 0) {
			int32_t size;
			fread(&size, sizeof(size), 1, fp);
			fseek(fp, size, SEEK_CUR);
		}
	}

	//Section G - TODO
	{
		struct Unknown {
			int32_t u1[2];
			float u2[7];
			int32_t u3[5];
			int32_t type;
		};

		int32_t count;
		fread(&count, sizeof(count), 1, fp);
		if (count != 0) return;
		for (int32_t i = 0; i < count; ++i) {
			Unknown u;
			fread(&u, sizeof(u), 1, fp);

			if (u.type == 2) {

			} else {

			}

		}
	}

	//Section F - TODO
	{
		int32_t count;
		fread(&count, sizeof(count), 1, fp);
		assert(count == 0);
		for (int32_t i = 0; i < count; ++i) {

		}
	}

	//Parse Mesh
	for (int32_t i = 0; i < head.lodCount; ++i) {
		int32_t meshCount;
		fread(&meshCount, sizeof(meshCount), 1, fp);
		meshes.resize(meshCount);
		for (int32_t j = 0; j < meshCount; ++j) {
			Mesh &mesh = meshes[j];

			struct MeshData {
				float u1[10];
				uint16_t u2[20];
				uint32_t matCount;
				uint32_t u3[2];
			};
			MeshData data;
			fread(&data, sizeof(data), 1, fp);

			mesh.vertexStride = data.u2[4];
			mesh.matID = data.u2[2];
			mesh.vertexCount = 1 + data.u2[18] - data.u2[17];
			mesh.totalVertexCount = data.u2[16];
			mesh.faceCount = data.u2[10];
			mesh.UVFlag = data.u2[3];
			mesh.scaleFlag = 0;
			mesh.boneMapID = data.u2[6];

			for (int32_t k = 0; k < data.matCount; ++k) {
				fseek(fp, 68, SEEK_CUR);

				uint32_t matlen;
				fread(&matlen, sizeof(matlen), 1, fp);
				mesh.mat = std::string(matlen, '\0');
				fread(&mesh.mat[0], 1, matlen, fp);
				seekpad(fp, 4);

				uint16_t u[2];
				fread(u, sizeof(uint16_t), 2, fp);
			}
		}
	}

	int32_t min, max;
	fread(&min, sizeof(min), 1, fp);
	fread(&max, sizeof(max), 1, fp);

	struct Model {

	};
	std::vector<Model> models(head.lodCount);

	for (int32_t i = 0; i < max; ++i) {
		int32_t meshCount;
		fread(&meshCount, sizeof(meshCount), 1, fp);

		for (int32_t j = 0; j < meshes.size(); ++j) {
			Mesh &mesh = meshes[j];

			mesh.buffer.resize(mesh.vertexCount * mesh.vertexStride / sizeof(uint16_t));
			fread(mesh.buffer.data(), sizeof(uint16_t), mesh.buffer.size(), fp);
			mesh.vbo = createVertexBuffer(mesh.buffer.data(), sizeof(uint16_t) * mesh.buffer.size(), BUFFER_STATIC);
		}

		//Faces
		fread(&a, sizeof(a), 1, fp);
		for (int32_t j = 0; j < meshes.size(); ++j) {
			Mesh &mesh = meshes[j];

			mesh.index.resize(mesh.faceCount * 3);
			fread(mesh.index.data(), sizeof(uint16_t), mesh.index.size(), fp);
			mesh.ibo = createVertexBuffer(mesh.index.data(), sizeof(uint16_t) * mesh.index.size(), BUFFER_STATIC);
		}

		seekpad(fp, 4);

		break;
	}

	fclose(fp);
}

void xbgFile::draw() {
	for (auto &mesh : meshes) {
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

		// Draw the triangles
		GLint id;
		glGetIntegerv(GL_CURRENT_PROGRAM, &id);
		glDrawElements(GL_TRIANGLES, mesh.faceCount * 3, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(0);
	}
}
