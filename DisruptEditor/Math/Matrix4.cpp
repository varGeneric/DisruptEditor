#include "Matrix4.h"

#include <assert.h>

mat4::mat4() {
	identity();
}

mat4::mat4(float s) {
	if(s == 1.f)
		identity();
	else
		m[0] = m[1] = m[2] = m[3] = s;
}

mat4::mat4(const float src[16]) {
	set(src);
}

mat4::mat4(float m00, float m01, float m02, float m03,
	float m04, float m05, float m06, float m07,
	float m08, float m09, float m10, float m11,
	float m12, float m13, float m14, float m15) {
	set(m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15);
}

void mat4::set(const float src[16]) {
	set(src[0], src[1], src[2], src[3],
		src[4], src[5], src[6], src[7],
		src[8], src[9], src[10], src[11],
		src[12], src[13], src[14], src[15]);
}

void mat4::set(float m00, float m01, float m02, float m03,
	float m04, float m05, float m06, float m07,
	float m08, float m09, float m10, float m11,
	float m12, float m13, float m14, float m15) {
	m[0][0] = m00;  m[0][1] = m01;  m[0][2] = m02;  m[0][3] = m03;
	m[1][0] = m04;  m[1][1] = m05;  m[1][2] = m06;  m[1][3] = m07;
	m[2][0] = m08;  m[2][1] = m09;  m[2][2] = m10;  m[2][3] = m11;
	m[3][0] = m12;  m[3][1] = m13;  m[3][2] = m14;  m[3][3] = m15;
}

mat4& mat4::identity() {
	m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
	m[0][1] = m[0][2] = m[0][3] = m[1][0] = m[1][2] = m[1][3] = m[2][0] = m[2][1] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.0f;
	return *this;
}

mat4 mat4::operator*(const mat4& m2) const {
	vec4 const SrcA0 = m[0];
	vec4 const SrcA1 = m[1];
	vec4 const SrcA2 = m[2];
	vec4 const SrcA3 = m[3];

	vec4 const SrcB0 = m2.m[0];
	vec4 const SrcB1 = m2.m[1];
	vec4 const SrcB2 = m2.m[2];
	vec4 const SrcB3 = m2.m[3];

	mat4 Result;
	Result[0] = SrcA0 * SrcB0[0] + SrcA1 * SrcB0[1] + SrcA2 * SrcB0[2] + SrcA3 * SrcB0[3];
	Result[1] = SrcA0 * SrcB1[0] + SrcA1 * SrcB1[1] + SrcA2 * SrcB1[2] + SrcA3 * SrcB1[3];
	Result[2] = SrcA0 * SrcB2[0] + SrcA1 * SrcB2[1] + SrcA2 * SrcB2[2] + SrcA3 * SrcB2[3];
	Result[3] = SrcA0 * SrcB3[0] + SrcA1 * SrcB3[1] + SrcA2 * SrcB3[2] + SrcA3 * SrcB3[3];
	return Result;
}

mat4& mat4::operator*=(const mat4& rhs) {
	*this = *this * rhs;
	return *this;
}

vec3 mat4::operator*(const vec3 & rhs) const {
	vec3 r;

	float fInvW = 1.0 / (m[3][0] * rhs.x + m[3][1] * rhs.y + m[3][2] * rhs.z + m[3][3]);

	r.x = (m[0][0] * rhs.x + m[0][1] * rhs.y + m[0][2] * rhs.z + m[0][3]) * fInvW;
	r.y = (m[1][0] * rhs.x + m[1][1] * rhs.y + m[1][2] * rhs.z + m[1][3]) * fInvW;
	r.z = (m[2][0] * rhs.x + m[2][1] * rhs.y + m[2][2] * rhs.z + m[2][3]) * fInvW;

	return r;
}

vec4 mat4::operator*(const vec4 & rhs) const {
	vec4 r;

	float fInvW = 1.0 / (m[3][0] * rhs.x + m[3][1] * rhs.y + m[3][2] * rhs.z + m[3][3] * rhs.w);

	r.x = (m[0][0] * rhs.x + m[0][1] * rhs.y + m[0][2] * rhs.z + m[0][3] * rhs.w) * fInvW;
	r.y = (m[1][0] * rhs.x + m[1][1] * rhs.y + m[1][2] * rhs.z + m[1][3] * rhs.w) * fInvW;
	r.z = (m[2][0] * rhs.x + m[2][1] * rhs.y + m[2][2] * rhs.z + m[2][3] * rhs.w) * fInvW;
	r.w = (m[3][0] * rhs.x + m[3][1] * rhs.y + m[3][2] * rhs.z + m[3][3] * rhs.w) * fInvW;

	return r;
}

mat4 mat4::operator*(const float &f) const {
	mat4 r;

	r.m[0][0] = m[0][0] * f;
	r.m[0][1] = m[0][1] * f;
	r.m[0][2] = m[0][2] * f;
	r.m[0][3] = m[0][3] * f;

	r.m[1][0] = m[1][0] * f;
	r.m[1][1] = m[1][1] * f;
	r.m[1][2] = m[1][2] * f;
	r.m[1][3] = m[1][3] * f;

	r.m[2][0] = m[2][0] * f;
	r.m[2][1] = m[2][1] * f;
	r.m[2][2] = m[2][2] * f;
	r.m[2][3] = m[2][3] * f;

	r.m[3][0] = m[3][0] * f;
	r.m[3][1] = m[3][1] * f;
	r.m[3][2] = m[3][2] * f;
	r.m[3][3] = m[3][3] * f;

	return r;
}

mat4 mat4::operator/(const mat4& n) const {
	return this->inverse() * n;
}

mat4& mat4::operator/=(const mat4& rhs) {
	*this = *this / rhs;
	return *this;
}

bool mat4::operator==(const mat4& n) const {
	return m[0] == n.m[0] && m[1] == n.m[1] && m[2] == n.m[2] && m[3] == n.m[3];
}

bool mat4::operator!=(const mat4& n) const {
	return m[0] == n.m[0] && m[1] == n.m[1] && m[2] == n.m[2] && m[3] == n.m[3];
}

mat4 mat4::inverse() const {
	// based on http://www.euclideanspace.com/maths/algebra/matrix/functions/inverse/fourD/index.htm
	const mat4 &me = *this;
	const float n11 = me[0][0], n21 = me[0][1], n31 = me[0][2], n41 = me[0][3],
		n12 = me[1][0], n22 = me[1][1], n32 = me[1][2], n42 = me[1][3],
		n13 = me[2][0], n23 = me[2][1], n33 = me[2][2], n43 = me[2][3],
		n14 = me[3][0], n24 = me[3][1], n34 = me[3][2], n44 = me[3][3],

		t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44,
		t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44,
		t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44,
		t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

	float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
	if (det == 0) {
		return mat4();
	}

	float detInv = 1 / det;
	mat4 te;
	te[0][0] = t11 * detInv;
	te[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * detInv;
	te[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * detInv;
	te[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * detInv;

	te[1][0] = t12 * detInv;
	te[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * detInv;
	te[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * detInv;
	te[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * detInv;

	te[2][0] = t13 * detInv;
	te[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * detInv;
	te[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * detInv;
	te[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * detInv;

	te[3][0] = t14 * detInv;
	te[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * detInv;
	te[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * detInv;
	te[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * detInv;

	return te;
}

float mat4::MINOR(int r0, int r1, int r2, int c0, int c1, int c2) const {
	return m[r0][c0] * (m[r1][c1] * m[r2][c2] - m[r2][c1] * m[r1][c2]) -
		m[r0][c1] * (m[r1][c0] * m[r2][c2] - m[r2][c0] * m[r1][c2]) +
		m[r0][c2] * (m[r1][c0] * m[r2][c1] - m[r2][c0] * m[r1][c1]);
}

float mat4::determinant() const {
	return m[0][0] * MINOR(1, 2, 3, 1, 2, 3) -
		m[0][1] * MINOR(1, 2, 3, 0, 2, 3) +
		m[0][2] * MINOR(1, 2, 3, 0, 1, 3) -
		m[0][3] * MINOR(1, 2, 3, 0, 1, 2);
}

mat4 mat4::adjoint() const {
	return mat4(MINOR(1, 2, 3, 1, 2, 3),
		-MINOR(0, 2, 3, 1, 2, 3),
		MINOR(0, 1, 3, 1, 2, 3),
		-MINOR(0, 1, 2, 1, 2, 3),

		-MINOR(1, 2, 3, 0, 2, 3),
		MINOR(0, 2, 3, 0, 2, 3),
		-MINOR(0, 1, 3, 0, 2, 3),
		MINOR(0, 1, 2, 0, 2, 3),

		MINOR(1, 2, 3, 0, 1, 3),
		-MINOR(0, 2, 3, 0, 1, 3),
		MINOR(0, 1, 3, 0, 1, 3),
		-MINOR(0, 1, 2, 0, 1, 3),

		-MINOR(1, 2, 3, 0, 1, 2),
		MINOR(0, 2, 3, 0, 1, 2),
		-MINOR(0, 1, 3, 0, 1, 2),
		MINOR(0, 1, 2, 0, 1, 2));
}

vec4 mat4::operator[](int index) const {
	assert(index >= 0 && index < 4);
	return m[index];
}

vec4& mat4::operator[](int index) {
	assert(index >= 0 && index < 4);
	return m[index];
}
