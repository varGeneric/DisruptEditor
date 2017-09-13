#include "Vector2.h"

#include <math.h>

ivec2::ivec2() { x = 0; y = 0; }
ivec2::ivec2(int s) {
	x = s, y = s;
}
ivec2::ivec2(int _x, int _y) { 
	x = _x; y = _y; 
}

ivec2::ivec2(const vec2 & v) : x(v.x), y(v.y) {}

vec2::vec2() {
	x = 0.f; y = 0.f;
}

vec2::vec2(float s) {
	x = s; y = s;
}

vec2::vec2(float _x, float _y) {
	x = _x; y = _y;
}

vec2::vec2(const ivec2 & v) : x(v.x), y(v.y) {}

vec2 vec2::operator+(const vec2& n) const {
	return vec2(x + n.x, y + n.y);
}

vec2& vec2::operator+=(const vec2& rhs) {
	*this = *this + rhs;
	return *this;
}

vec2 vec2::operator-(const vec2& n) const {
	return vec2(x - n.x, y - n.y);
}

vec2& vec2::operator-=(const vec2& rhs) {
	*this = *this - rhs;
	return *this;
}

vec2 vec2::operator*(const vec2& n) const {
	return vec2(x * n.x, y * n.y);
}

vec2& vec2::operator*=(const vec2& rhs) {
	*this = *this * rhs;
	return *this;
}

vec2 vec2::operator/(const vec2& n) const {
	return vec2(x / n.x, y / n.y);
}

vec2& vec2::operator/=(const vec2& rhs) {
	*this = *this / rhs;
	return *this;
}

float vec2::distance(const vec2 & other) const {
	return sqrtf((x - other.x) * (x - other.x) +
		(y - other.y) * (y - other.y));
}

float vec2::distanceSquared(const vec2 & other) const {
	return (x - other.x) * (x - other.x) +
		(y - other.y) * (y - other.y);
}

float vec2::magnitude() {
	return sqrtf((x * x) + (y * y));
}

vec2 vec2::normalize() {
	const float m = magnitude();
	return vec2(x / m, y / m);
}
