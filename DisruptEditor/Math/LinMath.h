#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix3.h"
#include "Matrix4.h"

#include <math.h>
#ifndef PI
#define PI 3.141592653f
#endif

static inline vec3 sqrt(vec3 const &n) {
	return vec3(sqrtf(n.x), sqrtf(n.y), sqrtf(n.z));
}

static inline vec3 inversesqrt(vec3 const &n) {
	return vec3(1.f) / sqrt(n);
}

static inline float dot(vec3 const &l, vec3 const &r) {
	vec3 tmp = l * r;
	return tmp.x + tmp.y + tmp.z;
}

static inline vec3 normalize(vec3 const &n) {
	return n * inversesqrt(dot(n, n));
}

static inline vec3 cross(vec3 const &l, vec3 const &r) {
	return vec3(l.y * r.z - r.y * l.z,
		l.z * r.x - r.z * l.x,
		l.x * r.y - r.x * l.y);
}

static inline mat4 QuatToMat4(const vec4 &q) {
	mat4 Result;

	float qxx(q.x * q.x);
	float qyy(q.y * q.y);
	float qzz(q.z * q.z);
	float qxz(q.x * q.z);
	float qxy(q.x * q.y);
	float qyz(q.y * q.z);
	float qwx(q.w * q.x);
	float qwy(q.w * q.y);
	float qwz(q.w * q.z);

	Result[0][0] = 1 - 2 * (qyy + qzz);
	Result[0][1] = 2 * (qxy + qwz);
	Result[0][2] = 2 * (qxz - qwy);

	Result[1][0] = 2 * (qxy - qwz);
	Result[1][1] = 1 - 2 * (qxx + qzz);
	Result[1][2] = 2 * (qyz + qwx);

	Result[2][0] = 2 * (qxz + qwy);
	Result[2][1] = 2 * (qyz - qwx);
	Result[2][2] = 1 - 2 * (qxx + qyy);

	return Result;
}

static inline mat4 MATlookAt(vec3 const & eye, vec3 const & center, vec3 const & up) {
	const vec3 f(normalize(center - eye));
	const vec3 s(normalize(cross(f, up)));
	const vec3 u(cross(s, f));

	mat4 Result;
	Result[0][0] = s.x;
	Result[1][0] = s.y;
	Result[2][0] = s.z;
	Result[0][1] = u.x;
	Result[1][1] = u.y;
	Result[2][1] = u.z;
	Result[0][2] =-f.x;
	Result[1][2] =-f.y;
	Result[2][2] =-f.z;
	Result[3][0] =-dot(s, eye);
	Result[3][1] =-dot(u, eye);
	Result[3][2] = dot(f, eye);
	return Result;
}

static inline mat4 MATperspective(float fovy, float aspect, float zNear, float zFar) {
	const float f = 1.f / tanf(fovy / 2.f);

	mat4 Result(0.f);
	Result[0][0] = f / aspect;
	Result[1][1] = f;
	Result[2][3] = -1.f;
	Result[2][2] = -(zFar + zNear) / (zFar - zNear);
	Result[3][2] = -(2.f * zFar * zNear) / (zFar - zNear);

	return Result;

}

static inline mat4 MATperspectiveInf(float fovy, float aspect, float zNear) {
	mat4 Result(0.f);
	Result[0][0] = fovy / aspect;
	Result[1][1] = fovy;
	Result[2][3] = -zNear;
	Result[3][2] = -1.f;

	return Result;
}

static inline mat4 MATrotate(mat4 const &m, float angle, vec3 const &v) {
	float const a = angle;
	float const c = cosf(a);
	float const s = sinf(a);
	
	vec3 axis(normalize(v));
	vec3 temp(vec3(1.f - c) * axis);
	
	mat4 Rotate;
	Rotate[0][0] = c + temp[0] * axis[0];
	Rotate[0][1] = 0 + temp[0] * axis[1] + s * axis[2];
	Rotate[0][2] = 0 + temp[0] * axis[2] - s * axis[1];
	
	Rotate[1][0] = 0 + temp[1] * axis[0] - s * axis[2];
	Rotate[1][1] = c + temp[1] * axis[1];
	Rotate[1][2] = 0 + temp[1] * axis[2] + s * axis[0];
	
	Rotate[2][0] = 0 + temp[2] * axis[0] + s * axis[1];
	Rotate[2][1] = 0 + temp[2] * axis[1] - s * axis[0];
	Rotate[2][2] = c + temp[2] * axis[2];
	
	mat4 Result;
	Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
	Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
	Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
	Result[3] = m[3];
	return Result;
}

static inline mat4 MATtranslate(mat4 const &m, vec3 const &v) {
	//GLM Version
	mat4 Result(m);
	Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
	return Result;
}

static inline mat4 MATscale(mat4 const &m, vec3 const &v) {
	mat4 Result;
	Result[0] = m[0] * v[0];
	Result[1] = m[1] * v[1];
	Result[2] = m[2] * v[2];
	Result[3] = m[3];
	return Result;
}

static inline mat4 MATcompose(const vec3 &translation, const vec4 &quaternion, const vec3 &scale) {
	mat4 matrix = QuatToMat4(quaternion);
	matrix = MATtranslate(matrix, translation);
	return MATscale(matrix, scale);
}

static inline float length2(vec3 const &m) {
	return dot(m, m);
}

static inline vec3 MATProject(const vec3 &pos, const mat4 &view, const mat4 &projection) {
	//Projection transform, the final row of projection matrix is always [0 0 -1 0]
	//so we optimize for that.
	vec4 fTempo = projection * vec4(pos, 1.f);
	fTempo.w = -pos.z;
	//The result normalizes between -1 and 1
	if (fTempo.w == 0.f)	//The w value
		return vec3(-5000.f, -5000.f, 0.f);
	fTempo.w = 1.f / fTempo.w;
	//Perspective division
	fTempo.x *= fTempo.w;
	fTempo.y *= fTempo.w;
	fTempo.z *= fTempo.w;
	//Window coordinates
	//Map x, y to range 0-1
	return vec3((fTempo.x * 0.5 + 0.5)*view[0][2] + view[0][0], (fTempo.y * 0.5 + 0.5)*view[0][3] + view[0][1], (1.f + fTempo.z)*0.5f);
}
