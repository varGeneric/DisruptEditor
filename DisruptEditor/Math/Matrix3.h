#pragma once
#include <SDL_assert.h>
#include "Vector3.h"

class mat3 {
public:
	mat3();
	mat3(float s) {
		if (s == 1.f)
			identity();
		else
			set(s);
	}
	mat3(const float src[9]) { set(src); }
	mat3(float m00, float m01, float m02, float m03, float m04, float m05, float m06, float m07, float m08) { set(m00, m01, m02, m03, m04, m05, m06, m07, m08); }
	void set(float s);
	void set(const float src[9]);
	void set(float m00, float m01, float m02, float m03, float m04, float m05, float m06, float m07, float m08);
	void identity();
	mat3 operator*(const mat3& rhs) const;
	mat3& operator*=(const mat3& rhs);

	vec3 operator*(const vec3& rhs) const;
	mat3 operator*(const float& rhs) const;

	mat3 operator/(const mat3& rhs) const;
	mat3& operator/=(const mat3& rhs);

	bool operator==(const mat3 & n) const;
	bool operator!=(const mat3 & n) const;

	mat3 inverse() const;

	vec3 operator[](int index) const { return m[index]; };
	vec3& operator[](int index) { return m[index]; };

	vec3 m[3];
};
