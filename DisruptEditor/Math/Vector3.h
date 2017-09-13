#pragma once

class vec3 {
public:
	vec3();
	vec3(float s);
	vec3(float _x, float _y, float _z);

	vec3 operator+(const vec3& n) const;
	vec3& operator+=(const vec3& rhs);
	vec3 operator-(const vec3& n) const;
	vec3& operator-=(const vec3& rhs);

	vec3 operator*(const vec3& n) const;
	vec3& operator*=(const vec3& rhs);
	vec3 operator/(const vec3& n) const;
	vec3& operator/=(const vec3& rhs);

	vec3 operator+(const float& n) const;
	vec3& operator+=(const float& rhs);
	vec3 operator-(const float& n) const;
	vec3& operator-=(const float& rhs);

	vec3 operator*(const float& n) const;
	vec3& operator*=(const float& rhs);
	vec3 operator/(const float& n) const;
	vec3& operator/=(const float& rhs);

	bool operator==(const vec3& n) const;
	bool operator!=(const vec3& n) const;

	float operator[](int index) const;
	float& operator[](int index);
	
	float distance(const vec3 &other) const;
	float distanceSquared(const vec3 &other) const;
	float magnitude() const;
	vec3 normalize() const;
	void normalize(float l);
	vec3 getNormal(const vec3 &other) const;
	float length() const;

	float x, y, z;
};

inline vec3 operator*(float s, const vec3& v) {
	return vec3(s * v.x, s * v.y, s * v.z);
}
