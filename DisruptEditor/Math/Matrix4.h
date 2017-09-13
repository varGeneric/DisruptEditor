#pragma once
#include "Vector3.h"
#include "Vector4.h"

class mat4 {
public:
	mat4();
	mat4(float s);
	mat4(const float src[16]);
	mat4(float m00, float m01, float m02, float m03,
		float m04, float m05, float m06, float m07,
		float m08, float m09, float m10, float m11,
		float m12, float m13, float m14, float m15);
	void set(const float src[16]);
	void set(float m00, float m01, float m02, float m03, float m04, float m05, float m06, float m07, float m08, float m09, float m10, float m11, float m12, float m13, float m14, float m15);
	mat4 & identity();
	mat4 operator*(const mat4& rhs) const;
	mat4& operator*=(const mat4& rhs);

	vec3 operator*(const vec3& rhs) const;
	vec4 operator*(const vec4& rhs) const;
	mat4 operator*(const float& rhs) const;

	mat4 operator/(const mat4& rhs) const;
	mat4& operator/=(const mat4& rhs);

	bool operator==(const mat4 & n) const;
	bool operator!=(const mat4 & n) const;

	mat4 inverse() const;
	float MINOR(int r0, int r1, int r2, int c0, int c1, int c2) const;
	float determinant() const;

	mat4 adjoint() const;
	mat4 swapMajor() const;

	vec4 operator[](int index) const;
	vec4& operator[](int index);

	vec4 m[4];
};
