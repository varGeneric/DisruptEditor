#include "Vector4.h"

#include <SDL_assert.h>

vec4::vec4() {
	x = 0.f; y = 0.f; z = 0.f; w = 0.f;
}

vec4::vec4(float s) {
	x = s; y = s; z = s; w = s;
}

vec4::vec4(float _x, float _y, float _z, float _w) {
	x = _x; y = _y; z = _z; w = _w;
}

vec4::vec4(vec3 s, float _w) {
	x = s.x; y = s.y; z = s.z; w = _w;
}

vec4 vec4::operator+(const vec4& n) const {
	return vec4(x + n.x, y + n.y, z + n.z, w + n.w);
}

vec4& vec4::operator+=(const vec4& rhs) {
	*this = *this + rhs;
	return *this;
}

vec4 vec4::operator-(const vec4& n) const {
	return vec4(x - n.x, y - n.y, z - n.z, w  - n.w);
}

vec4& vec4::operator-=(const vec4& rhs) {
	*this = *this - rhs;
	return *this;
}

vec4 vec4::operator*(const vec4& n) const {
	return vec4(x * n.x, y * n.y, z * n.z, w * n.w);
}

vec4& vec4::operator*=(const vec4& rhs) {
	*this = *this * rhs;
	return *this;
}

vec4 vec4::operator/(const vec4& n) const {
	return vec4(x / n.x, y / n.y, z / n.z, w / n.w);
}

vec4& vec4::operator/=(const vec4& rhs) {
	*this = *this / rhs;
	return *this;
}

vec4 vec4::operator+(const float& n) const {
	return vec4(x + n, y + n, z + n, w + n);
}

vec4& vec4::operator+=(const float& rhs) {
	*this = *this + rhs;
	return *this;
}

vec4 vec4::operator-(const float& n) const {
	return vec4(x - n, y - n, z - n, w - n);
}

vec4& vec4::operator-=(const float& rhs) {
	*this = *this - rhs;
	return *this;
}

vec4 vec4::operator*(const float& n) const {
	return vec4(x * n, y * n, z * n, w * n);
}

vec4& vec4::operator*=(const float& rhs) {
	*this = *this * rhs;
	return *this;
}

vec4 vec4::operator/(const float& n) const {
	return vec4(x / n, y / n, z / n, w / n);
}

vec4& vec4::operator/=(const float& rhs) {
	*this = *this / rhs;
	return *this;
}

float vec4::operator[](int index) const {
	SDL_assert_release(index >= 0 && index < 4);
	switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			return w;
	}
}

float& vec4::operator[](int index) {
	SDL_assert_release(index >= 0 && index < 4);
	switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			return w;
	}
}

bool vec4::operator==(const vec4 & n) const {
	return x == n.x && y == n.y && z == n.z && w == n.w;
}

bool vec4::operator!=(const vec4 & n) const {
	return x != n.x || y != n.y || z != n.z || w != n.w;
}
