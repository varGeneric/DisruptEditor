#pragma once
#include "Matrix4.h"
#include "AABB.h"

#include "LinearMath/btVector3.h"

struct Plane {
	float a, b, c, d;
};

struct Frustum {
	Frustum(const mat4 &ModelView) {
		float *mat = (float*)(&ModelView);
		// Left Plane
		// col4 + col1
		planes[0].a = mat[3] + mat[0];
		planes[0].b = mat[7] + mat[4];
		planes[0].c = mat[11] + mat[8];
		planes[0].d = mat[15] + mat[12];

		// Right Plane
		// col4 - col1
		planes[1].a = mat[3] - mat[0];
		planes[1].b = mat[7] - mat[4];
		planes[1].c = mat[11] - mat[8];
		planes[1].d = mat[15] - mat[12];

		// Bottom Plane
		// col4 + col2
		planes[2].a = mat[3] + mat[1];
		planes[2].b = mat[7] + mat[5];
		planes[2].c = mat[11] + mat[9];
		planes[2].d = mat[15] + mat[13];

		// Top Plane
		// col4 - col2
		planes[3].a = mat[3] - mat[1];
		planes[3].b = mat[7] - mat[5];
		planes[3].c = mat[11] - mat[9];
		planes[3].d = mat[15] - mat[13];

		// Near Plane
		// col4 + col3
		planes[4].a = mat[3] + mat[2];
		planes[4].b = mat[7] + mat[6];
		planes[4].c = mat[11] + mat[10];
		planes[4].d = mat[15] + mat[14];

		// Far Plane
		// col4 - col3
		planes[5].a = mat[3] - mat[2];
		planes[5].b = mat[7] - mat[6];
		planes[5].c = mat[11] - mat[10];
		planes[5].d = mat[15] - mat[14];
	}
	bool intersectAABB(const btVector3 &min, const btVector3 &max) {
		btVector3 box[] = { min, max };

		// We only need to do 6 point-plane tests
		for (int i = 0; i < 6; ++i) {
			// This is the current plane
			const Plane &p = planes[i];

			// p-vertex selection (with the index trick)
			// According to the plane normal we can know the
			// indices of the positive vertex
			const int px = (int)(p.a > 0.0f);
			const int py = (int)(p.b > 0.0f);
			const int pz = (int)(p.c > 0.0f);

			// Dot product
			// project p-vertex on plane normal
			// (How far is p-vertex from the origin)
			const float dp =
				(p.a*box[px].x()) +
				(p.b*box[py].y()) +
				(p.c*box[pz].z());

			// Doesn't intersect if it is behind the plane
			if (dp < -p.d)
				return false;
		}
		return true;
	}
	bool intersectAABB(const AABB &aabb) {
		vec3 box[] = { aabb.min, aabb.max };

		// We only need to do 6 point-plane tests
		for (int i = 0; i < 6; ++i) {
			// This is the current plane
			const Plane &p = planes[i];

			// p-vertex selection (with the index trick)
			// According to the plane normal we can know the
			// indices of the positive vertex
			const int px = (int)(p.a > 0.0f);
			const int py = (int)(p.b > 0.0f);
			const int pz = (int)(p.c > 0.0f);

			// Dot product
			// project p-vertex on plane normal
			// (How far is p-vertex from the origin)
			const float dp =
				(p.a*box[px].x) +
				(p.b*box[py].y) +
				(p.c*box[pz].z);

			// Doesn't intersect if it is behind the plane
			if (dp < -p.d)
				return false;
		}
		return true;
	}
	Plane planes[6];
};