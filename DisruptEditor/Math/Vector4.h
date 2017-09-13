#pragma once
#include "Vector3.h"

class vec4 {
public:
	vec4();
	vec4(float s);
	vec4(float _x, float _y, float _z, float _w);
	vec4(vec3 s, float _w);

	vec4 operator+(const vec4 & n) const;
	vec4& operator+=(const vec4& rhs);
	vec4 operator-(const vec4 & n) const;
	vec4& operator-=(const vec4& rhs);

	vec4 operator*(const vec4 & n) const;
	vec4& operator*=(const vec4& rhs);
	vec4 operator/(const vec4 & n) const;
	vec4& operator/=(const vec4& rhs);

	vec4 operator+(const float& n) const;
	vec4& operator+=(const float& rhs);
	vec4 operator-(const float& n) const;
	vec4& operator-=(const float& rhs);

	vec4 operator*(const float& n) const;
	vec4& operator*=(const float& rhs);
	vec4 operator/(const float& n) const;
	vec4& operator/=(const float& rhs);

	float operator[](int index) const;
	float& operator[](int index);
	bool operator==(const vec4& n) const;
	bool operator!=(const vec4& n) const;
	
	float x, y, z, w;
};
