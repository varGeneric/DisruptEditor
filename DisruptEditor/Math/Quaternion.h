#pragma once
#include "Vector3.h"
#include <math.h>
#include <float.h>

struct Quat {
	// Default constructor does nothing for performance.
	Quat() {}

	// Set this quaternion from four values.
	Quat(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

	// Set this quaternion from an axis and an angle 
	// of rotation about the axis.
	Quat(const vec3& axis, float angle) {
		Set(axis, angle);
	}

	// Write an indexed value to this quaternion.
	float& operator[](int i) {
		return (&x)[i];
	}

	// Read an indexed value from this quaternion.
	float operator[](int i) const {
		return (&x)[i];
	}

	// Add a quaternion to this quaternion.
	void operator+=(const Quat& q) {
		x += q.x;
		y += q.y;
		z += q.z;
		w += q.w;
	}

	// Subtract a quaternion from this quaternion.
	void operator-=(const Quat& q) {
		x -= q.x;
		y -= q.y;
		z -= q.z;
		w -= q.w;
	}

	// Set this quaternion to identity.
	void SetIdentity() {
		x = y = z = 0.0f;
		w = 1.0f;
	}

	// Set this quaternion from four values.
	void Set(float _x, float _y, float _z, float _w) {
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}

	// Convert this quaternion to the unit quaternion. Return the length.
	float Normalize() {
		float length = sqrt(x * x + y * y + z * z + w * w);
		if (length > FLT_EPSILON) {
			float s = 1.0f / length;
			x *= s;
			y *= s;
			z *= s;
			w *= s;
		}
		return length;
	}

	// Set this quaternion from an axis and full angle 
	// of rotation about the axis.
	void Set(const vec3& axis, float angle) {
		// half angle
		float theta = 0.5f * angle;

		float sine = sin(theta);
		x = sine * axis.x;
		y = sine * axis.y;
		z = sine * axis.z;

		w = cos(theta);
	}

	// If this quaternion represents an orientation output 
	// the axis and angle of rotation about the axis.
	void GetAxisAngle(vec3* axis, float* angle) const {
		// sin^2 = 1 - cos^2
		// sin = sqrt( sin^2 ) = ||v||
		// axis = v / sin
		vec3 v(x, y, z);
		float sine = v.length();
		*axis = vec3();
		if (sine > FLT_EPSILON) {
			float s = 1.0f / sine;
			*axis = s * v;
		}

		// cosine check
		float cosine = clamp(w, -1.0f, 1.0f);
		// half angle
		float theta = acos(cosine);
		// full angle
		*angle = 2.0f * theta;
	}

	float x, y, z, w;
};
