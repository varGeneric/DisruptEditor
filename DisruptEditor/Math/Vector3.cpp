#include "Vector3.h"

#include <assert.h>
#include <math.h>

vec3::vec3() {
	x = 0.f; y = 0.f; z = 0.f;
}

vec3::vec3(float s) {
	x = s; y = s; z = s;
}

vec3::vec3(float _x, float _y, float _z) {
	x = _x; y = _y; z = _z;
}

vec3 vec3::operator+(const vec3& n) const {
	return vec3(x + n.x, y + n.y, z + n.z);
}

vec3& vec3::operator+=(const vec3& rhs) {
	*this = *this + rhs;
	return *this;
}

vec3 vec3::operator-(const vec3& n) const {
	return vec3(x - n.x, y - n.y, z - n.z);
}

vec3& vec3::operator-=(const vec3& rhs) {
	*this = *this - rhs;
	return *this;
}

vec3 vec3::operator*(const vec3& n) const {
	return vec3(x * n.x, y * n.y, z * n.z);
}

vec3& vec3::operator*=(const vec3& rhs) {
	*this = *this * rhs;
	return *this;
}

vec3 vec3::operator/(const vec3& n) const {
	return vec3(x / n.x, y / n.y, z / n.z);
}

vec3& vec3::operator/=(const vec3& rhs) {
	*this = *this / rhs;
	return *this;
}

vec3 vec3::operator+(const float& n) const {
	return vec3(x + n, y + n, z + n);
}

vec3& vec3::operator+=(const float& rhs) {
	*this = *this + rhs;
	return *this;
}

vec3 vec3::operator-(const float& n) const {
	return vec3(x - n, y - n, z - n);
}

vec3& vec3::operator-=(const float& rhs) {
	*this = *this - rhs;
	return *this;
}

vec3 vec3::operator*(const float& n) const {
	return vec3(x * n, y * n, z * n);
}

vec3& vec3::operator*=(const float& rhs) {
	*this = *this * rhs;
	return *this;
}

vec3 vec3::operator/(const float& n) const {
	return vec3(x / n, y / n, z / n);
}

vec3& vec3::operator/=(const float& rhs) {
	*this = *this / rhs;
	return *this;
}

bool vec3::operator==(const vec3 & n) const {
	return x == n.x && y == n.y && z == n.z;
}

bool vec3::operator!=(const vec3 & n) const {
	return x != n.x || y != n.y || z != n.z;
}

float vec3::operator[](int index) const {
	assert(index >= 0 && index < 3);
	switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		default:
			return z;
	}
}

float& vec3::operator[](int index) {
	assert(index >= 0 && index < 3);
	switch (index) {
		case 0:
		return x;
		case 1:
		return y;
		default:
		return z;
	}
}

float vec3::distance(const vec3 & other) const {
	return sqrtf((x - other.x) * (x - other.x) +
		(y - other.y) * (y - other.y) +
		(z - other.z) * (z - other.z));
}

float vec3::distanceSquared(const vec3 & other) const {
	return (x - other.x) * (x - other.x) +
		(y - other.y) * (y - other.y) +
		(z - other.z) * (z - other.z);
}

float vec3::magnitude() const {
	return sqrtf((x * x) + (y * y) + (z * z));
}

vec3 vec3::normalize() const {
	const float m = magnitude();
	return vec3(x / m, y / m, z / m);
}

void vec3::normalize(float l) {
	float length = this->length();
	float invLength = l / length;
	x *= invLength;
	y *= invLength;
	z *= invLength;
}

vec3 vec3::getNormal(const vec3 &other) const {
	return (*this - other).normalize();
}

float vec3::length() const {
	return sqrtf(x*x + y*y + z*z);
}
