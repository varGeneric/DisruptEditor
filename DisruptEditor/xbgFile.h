#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include "GLHelper.h"

class xbgFile {
public:
	xbgFile();
	~xbgFile();
	void open(const char* file);

	struct Mesh {
		uint16_t vertexStride, matID, vertexCount, totalVertexCount, faceCount, UVFlag, scaleFlag, boneMapID;
		std::string mat;
		std::vector<uint16_t> buffer, index;
		VertexBuffer vbo, ibo;
	};
	std::vector<Mesh> meshes;
	void draw();
};

