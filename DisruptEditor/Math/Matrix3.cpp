#include "Matrix3.h"

mat3::mat3() {
	identity();
}

void mat3::set(const float src[16]) {
	set(src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7], src[8]);
}

void mat3::set(float m00, float m01, float m02,
	float m03, float m04, float m05,
	float m06, float m07, float m08) {
	m[0][0] = m00;  m[0][1] = m01;  m[0][2] = m02;
	m[1][0] = m03;  m[1][1] = m04;  m[1][2] = m05;
	m[2][0] = m06;  m[2][1] = m07;  m[2][2] = m08;
}

void mat3::identity() {
	m[0][0] = m[1][1] = m[2][2] = 1.0f;
	m[0][1] = m[0][2] = m[1][0] = m[1][2] = m[2][0] = m[2][1] = 0.0f;
}

mat3 mat3::operator*(const mat3& m2) const {
	const vec3 SrcA0 = m[0];
	const vec3 SrcA1 = m[1];
	const vec3 SrcA2 = m[2];
	const vec3 SrcA3 = m[3];

	const vec3 SrcB0 = m2.m[0];
	const vec3 SrcB1 = m2.m[1];
	const vec3 SrcB2 = m2.m[2];
	const vec3 SrcB3 = m2.m[3];

	mat3 Result;
	Result[0] = SrcA0 * SrcB0[0] + SrcA1 * SrcB0[1] + SrcA2 * SrcB0[2] + SrcA3 * SrcB0[3];
	Result[1] = SrcA0 * SrcB1[0] + SrcA1 * SrcB1[1] + SrcA2 * SrcB1[2] + SrcA3 * SrcB1[3];
	Result[2] = SrcA0 * SrcB2[0] + SrcA1 * SrcB2[1] + SrcA2 * SrcB2[2] + SrcA3 * SrcB2[3];
	return Result;
}

mat3& mat3::operator*=(const mat3& rhs) {
	*this = *this * rhs;
	return *this;
}

vec3 mat3::operator*(const vec3 & rhs) const {
	vec3 r;

	float fInvW = 1.0 / (m[3][0] * rhs.x + m[3][1] * rhs.y + m[3][2] * rhs.z + m[3][3]);

	r.x = (m[0][0] * rhs.x + m[0][1] * rhs.y + m[0][2] * rhs.z + m[0][3]) * fInvW;
	r.y = (m[1][0] * rhs.x + m[1][1] * rhs.y + m[1][2] * rhs.z + m[1][3]) * fInvW;
	r.z = (m[2][0] * rhs.x + m[2][1] * rhs.y + m[2][2] * rhs.z + m[2][3]) * fInvW;

	return r;
}

mat3 mat3::operator*(const float &f) const {
	mat3 r;

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

mat3 mat3::operator/(const mat3& n) const {
	return this->inverse() * n;
}

mat3& mat3::operator/=(const mat3& rhs) {
	*this = *this / rhs;
	return *this;
}

bool mat3::operator==(const mat3& n) const {
	return m[0] == n.m[0] && m[1] == n.m[1] && m[2] == n.m[2] && m[3] == n.m[3];
}

bool mat3::operator!=(const mat3& n) const {
	return m[0] == n.m[0] && m[1] == n.m[1] && m[2] == n.m[2] && m[3] == n.m[3];
}

mat3 mat3::inverse() const {
	// based on http://www.euclideanspace.com/maths/algebra/matrix/functions/inverse/fourD/index.htm
	const mat3 &me = *this;
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
		return mat3();
	}

	float detInv = 1 / det;
	mat3 te;
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