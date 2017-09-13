#pragma once

class vec2;

class ivec2 {
public:
	ivec2();
	ivec2(int s);
	ivec2(int _x, int _y);
	ivec2(const vec2 &v);

	bool operator==(const ivec2 &rhs) {
		return x == rhs.x && y == rhs.y;
	}

	int x, y;
};

class vec2 {
public:
	vec2();
	vec2(float s);
	vec2(float _x, float _y);
	vec2(const ivec2 &v);

	vec2 operator+(const vec2 & n) const;
	vec2& operator+=(const vec2& rhs);
	vec2 operator-(const vec2 & n) const;
	vec2& operator-=(const vec2& rhs);

	vec2 operator*(const vec2 & n) const;
	vec2& operator*=(const vec2& rhs);
	vec2 operator/(const vec2 & n) const;
	vec2& operator/=(const vec2& rhs);

	float distance(const vec2 &other) const;
	float distanceSquared(const vec2 &other) const;
	float magnitude();
	vec2 normalize();

	float x, y;
};
