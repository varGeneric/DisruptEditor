#pragma once

#include <stdint.h>
#include "Vector.h"
#include <string>
#include "GLHelper.h"

class xbgFile {
public:
	void open(const char* file);

	struct Mesh {
		uint16_t vertexStride, matID, vertexCount, totalVertexCount, faceCount, UVFlag, scaleFlag, boneMapID;
		std::string mat;
		Vector<uint16_t> buffer, index;
		VertexBuffer vbo, ibo;
	};
	Vector<Mesh> meshes;

	struct Material {
		uint32_t hash;
		std::string file;
	};
	Vector<Material> materials;

	void draw();
};

