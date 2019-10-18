#include "Wasabi/Core/WMath.h"
#include <memory>

WMatrix::WMatrix() {
	for (int i = 0; i < 16; i++)
		mat[i] = i % 4 == i / 4 ? 1.0f : 0.0f;
}
WMatrix::WMatrix(float f11, float f12, float f13, float f14,
	float f21, float f22, float f23, float f24,
	float f31, float f32, float f33, float f34,
	float f41, float f42, float f43, float f44) {
	mat[0]  = f11;
	mat[1]  = f12;
	mat[2]  = f13;
	mat[3]  = f14;
	mat[4]  = f21;
	mat[5]  = f22;
	mat[6]  = f23;
	mat[7]  = f24;
	mat[8]  = f31;
	mat[9]  = f32;
	mat[10] = f33;
	mat[11] = f34;
	mat[12] = f41;
	mat[13] = f42;
	mat[14] = f43;
	mat[15] = f44;
}
WMatrix WMatrix::operator+ (const WMatrix m) const {
	WMatrix out = *this;
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++)
			out(i, j) += m(i, j);
	return out;
}
WMatrix WMatrix::operator- (const WMatrix m) const {
	WMatrix out = *this;
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++)
			out(i, j) -= m(i, j);
	return out;
}
WMatrix WMatrix::operator* (const WMatrix m) const {
	WMatrix out;
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++) {
			float fSum = 0.0f;
			for (uint32_t k = 0; k < 4; k++)
				fSum += (*this)(i, k) * m(k, j);
			out(i, j) = fSum;
		}
	return out;
}
WMatrix WMatrix::operator/ (const WMatrix m) const {
	WMatrix out;
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++) {
			float fSum = 0.0f;
			for (uint32_t k = 0; k < 4; k++)
				fSum += (*this)(i, k) / m(k, j);
			out(i, j) = fSum;
		}
	return out;
}
WMatrix WMatrix::operator* (const float f) const {
	WMatrix out = *this;
	for (uint32_t i = 0; i < 4; i++) {
		for (uint32_t j = 0; j < 4; j++) {
			out(i, j) *= f;
		}
	}
	return out;
}
WMatrix WMatrix::operator/ (const float f) const {
	WMatrix out = *this;
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++)
			out(i, j) /= f;
	return out;
}
void WMatrix::operator+= (const WMatrix m) {
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++)
			(*this)(i, j) += m(i, j);
}
void WMatrix::operator-= (const WMatrix m) {
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++)
			(*this)(i, j) -= m(i, j);
}
void WMatrix::operator*= (const WMatrix m) {
	WMatrix old = (*this);
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++) {
			float fSum = 0.0f;
			for (uint32_t k = 0; k < 4; k++)
				fSum += old(i, k) * m(k, j);
			(*this)(i, j) = fSum;
		}
}
void WMatrix::operator/= (const WMatrix m) {
	WMatrix old;
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++) {
			float fSum = 0.0f;
			for (uint32_t k = 0; k < 4; k++)
				fSum += old(i, k) / m(k, j);
			(*this)(i, j) = fSum;
		}
}
void WMatrix::operator*= (const float f) {
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++)
			(*this)(i, j) *= f;
}
void WMatrix::operator/= (const float f) {
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++)
			(*this)(i, j) /= f;
}
float& WMatrix::operator() (const uint32_t row, const uint32_t col) {
	return mat[row * 4 + col];
}
float WMatrix::operator() (const uint32_t row, const uint32_t col) const {
	return mat[row * 4 + col];
}
float& WMatrix::operator[] (const uint32_t index) {
	return mat[index];
}
float WMatrix::operator[] (const uint32_t index) const {
	return mat[index];
}

WMatrix WMatrixTranspose(const WMatrix m) {
	WMatrix out;
	for (uint32_t i = 0; i < 4; i++)
		for (uint32_t j = 0; j < 4; j++)
			out(i, j) = m(j, i);
	return out;
}
WMatrix WMatrixInverse(const WMatrix m) {
	WMatrix out;

	float s0 = m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1);
	float s1 = m(0, 0) * m(1, 2) - m(1, 0) * m(0, 2);
	float s2 = m(0, 0) * m(1, 3) - m(1, 0) * m(0, 3);
	float s3 = m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2);
	float s4 = m(0, 1) * m(1, 3) - m(1, 1) * m(0, 3);
	float s5 = m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3);

	float c5 = m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3);
	float c4 = m(2, 1) * m(3, 3) - m(3, 1) * m(2, 3);
	float c3 = m(2, 1) * m(3, 2) - m(3, 1) * m(2, 2);
	float c2 = m(2, 0) * m(3, 3) - m(3, 0) * m(2, 3);
	float c1 = m(2, 0) * m(3, 2) - m(3, 0) * m(2, 2);
	float c0 = m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1);

	// Should check for 0 determinant
	float invdet = 1.0f / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);

	out(0, 0) = (m(1, 1) * c5 - m(1, 2) * c4 + m(1, 3) * c3) * invdet;
	out(0, 1) = (-m(0, 1) * c5 + m(0, 2) * c4 - m(0, 3) * c3) * invdet;
	out(0, 2) = (m(3, 1) * s5 - m(3, 2) * s4 + m(3, 3) * s3) * invdet;
	out(0, 3) = (-m(2, 1) * s5 + m(2, 2) * s4 - m(2, 3) * s3) * invdet;

	out(1, 0) = (-m(1, 0) * c5 + m(1, 2) * c2 - m(1, 3) * c1) * invdet;
	out(1, 1) = (m(0, 0) * c5 - m(0, 2) * c2 + m(0, 3) * c1) * invdet;
	out(1, 2) = (-m(3, 0) * s5 + m(3, 2) * s2 - m(3, 3) * s1) * invdet;
	out(1, 3) = (m(2, 0) * s5 - m(2, 2) * s2 + m(2, 3) * s1) * invdet;

	out(2, 0) = (m(1, 0) * c4 - m(1, 1) * c2 + m(1, 3) * c0) * invdet;
	out(2, 1) = (-m(0, 0) * c4 + m(0, 1) * c2 - m(0, 3) * c0) * invdet;
	out(2, 2) = (m(3, 0) * s4 - m(3, 1) * s2 + m(3, 3) * s0) * invdet;
	out(2, 3) = (-m(2, 0) * s4 + m(2, 1) * s2 - m(2, 3) * s0) * invdet;

	out(3, 0) = (-m(1, 0) * c3 + m(1, 1) * c1 - m(1, 2) * c0) * invdet;
	out(3, 1) = (m(0, 0) * c3 - m(0, 1) * c1 + m(0, 2) * c0) * invdet;
	out(3, 2) = (-m(3, 0) * s3 + m(3, 1) * s1 - m(3, 2) * s0) * invdet;
	out(3, 3) = (m(2, 0) * s3 - m(2, 1) * s1 + m(2, 2) * s0) * invdet;

	return out;
}

float WVec2Dot(const WVector2 v1, const WVector2 v2) {
	return v1.x * v2.x + v1.y * v2.y;
}
WVector2 WVec2Normalize(const WVector2 v) {
	return v / WVec2Length(v);
}
float WVec2Length(const WVector2 v) {
	return sqrtf(v.x * v.x + v.y * v.y);
}
float WVec2LengthSq(const WVector2 v) {
	return (v.x * v.x + v.y * v.y);
}
WVector2 WVec2Lerp(const WVector2 v1, const WVector2 v2, const float fLerpVal) {
	return v1 + (v2 - v1) * fLerpVal;
}
WVector4 WVec2Transform(const WVector2 v, const WMatrix m) {
	return WVec4Transform(WVector4(v.x, v.y, 0, 1), m);
}
WVector2 WVec2TransformCoord(const WVector2 v, const WMatrix m) {
	WVector4 out = WVec4Transform(WVector4(v.x, v.y, 0, 1), m);
	return WVector2(out.x, out.y) / out.w;
}
WVector2 WVec2TransformNormal(const WVector2 v, const WMatrix m) {
	WVector4 out = WVec4Transform(WVector4(v.x, v.y, 0, 0), m);
	return WVector2(out.x, out.y);
}

float WVec3Dot(const WVector3 v1, const WVector3 v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
WVector3 WVec3Cross(const WVector3 v1, const WVector3 v2) {
	WVector3 v;
	v.x = v1.y * v2.z - v1.z * v2.y;
	v.y = v1.z * v2.x - v1.x * v2.z;
	v.z = v1.x * v2.y - v1.y * v2.x;
	return v;
}
WVector3 WVec3Normalize(const WVector3 v) {
	return v / WVec3Length(v);
}
float WVec3Length(const WVector3 v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
float WVec3LengthSq(const WVector3 v) {
	return (v.x * v.x + v.y * v.y + v.z * v.z);
}
WVector3 WVec3Lerp(const WVector3 v1, const WVector3 v2, const float fLerpVal) {
	return v1 + (v2 - v1) * fLerpVal;
}
WVector4 WVec3Transform(const WVector3 v, const WMatrix m) {
	return WVec4Transform(WVector4(v.x, v.y, v.z, 1), m);
}
WVector3 WVec3TransformCoord(const WVector3 v, const WMatrix m) {
	WVector4 out = WVec4Transform(WVector4(v.x, v.y, v.z, 1), m);
	return WVector3(out.x, out.y, out.z) / out.w;
}
WVector3 WVec3TransformNormal(const WVector3 v, const WMatrix m) {
	WVector4 out = WVec4Transform(WVector4(v.x, v.y, v.z, 0), m);
	return WVector3(out.x, out.y, out.z);
}

float WVec4Dot(const WVector4 v1, const WVector4 v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}
WVector4 WVec4Normalize(const WVector4 v) {
	return v / WVec4Length(v);
}
float WVec4Length(const WVector4 v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}
float WVec4LengthSq(const WVector4 v) {
	return (v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}
WVector4 WVec4Lerp(const WVector4 v1, const WVector4 v2, const float fLerpVal) {
	return v1 + (v2 - v1) * fLerpVal;
}
WVector4 WVec4Transform(const WVector4 v, const WMatrix m) {
	WVector4 out;
	out.x = v.x * m(0, 0) + v.y * m(1, 0) + v.z * m(2, 0) + v.w * m(3, 0);
	out.y = v.x * m(0, 1) + v.y * m(1, 1) + v.z * m(2, 1) + v.w * m(3, 1);
	out.z = v.x * m(0, 2) + v.y * m(1, 2) + v.z * m(2, 2) + v.w * m(3, 2);
	out.w = v.x * m(0, 3) + v.y * m(1, 3) + v.z * m(2, 3) + v.w * m(3, 3);
	return out;
}

WMatrix WRotationMatrixX(const float fAngle) {
	return WMatrix(1.0f, 0, 0, 0,
		0, cosf(fAngle), sinf(fAngle), 0,
		0, -sinf(fAngle), cosf(fAngle), 0,
		0, 0, 0, 1.0f);
}
WMatrix WRotationMatrixY(const float fAngle) {
	return WMatrix(cosf(fAngle), 0, -sinf(fAngle), 0,
		0, 1.0f, 0, 0,
		sinf(fAngle), 0, cosf(fAngle), 0,
		0, 0, 0, 1.0f);
}
WMatrix WRotationMatrixZ(const float fAngle) {
	return WMatrix(cosf(fAngle), sinf(fAngle), 0, 0,
		-sinf(fAngle), cosf(fAngle), 0, 0,
		0, 0, 1.0f, 0,
		0, 0, 0, 1.0f);
}
WMatrix WRotationMatrixAxis(const WVector3 axis, const float fAngle) {
	float c = cosf(fAngle);
	float s = sinf(fAngle);
	float t = 1 - c;
	float X = axis.x;
	float Y = axis.y;
	float Z = axis.z;

	return WMatrix(t*X*X + c, t*X*Y + s*Z, t*X*Z - s*Y, 0,
		t*X*Y - s*Z, t*Y*Y + c, t*Y*Z + s*X, 0,
		t*X*Z + s*Y, t*Y*Z - s*X, t*Z*Z + c, 0,
		0, 0, 0, 1);
}
WMatrix WScalingMatrix(const float fX, const float fY, const float fZ) {
	WMatrix out = WMatrix();
	out(0, 0) = fX;
	out(1, 1) = fY;
	out(2, 2) = fZ;
	return out;
}
WMatrix WScalingMatrix(const WVector3 scale) {
	WMatrix out = WMatrix();
	out(0, 0) = scale.x;
	out(1, 1) = scale.y;
	out(2, 2) = scale.z;
	return out;
}
WMatrix WTranslationMatrix(const float fX, const float fY, const float fZ) {
	WMatrix out = WMatrix();
	out(3, 0) = fX;
	out(3, 1) = fY;
	out(3, 2) = fZ;
	return out;
}
WMatrix WTranslationMatrix(const WVector3 pos) {
	WMatrix out = WMatrix();
	out(3, 0) = pos.x;
	out(3, 1) = pos.y;
	out(3, 2) = pos.z;
	return out;
}
WMatrix WPerspectiveProjMatrix(const float w, const float h,
	const float zn, const float zf) {
	return WMatrix(2.0f*zn / w, 0, 0, 0,
		0, 2.0f*zn / h, 0, 0,
		0, 0, zf / (zf - zn), 1,
		0, 0, zn*zf / (zn - zf), 0);
}
WMatrix WPerspectiveProjMatrixFOV(const float fFOV, const float fAspect,
	const float zn, const float zf) {
	float yScale = 1.0f / tanf(W_DEGTORAD(fFOV) / 2.0f);
	float xScale = yScale / fAspect;
	return WMatrix(xScale, 0, 0, 0,
		0, yScale, 0, 0,
		0, 0, zf / (zf - zn), 1,
		0, 0, zn*zf / (zn - zf), 0);
}
WMatrix WOrthogonalProjMatrix(const float w, const float h,
	const float zn, const float zf) {
	return WMatrix(2.0f / w, 0, 0, 0,
		0, 2.0f / h, 0, 0,
		0, 0, 1.0f / (zf - zn), 0,
		0, 0, zn / (zn - zf), 1);

}
float WPlaneDotCoord(const WPlane plane, const WVector3 vec) {
	return plane.a*vec.x + plane.b*vec.y + plane.c*vec.z + plane.d * 1;
}
float WPlaneDotNormal(const WPlane plane, const WVector3 vec) {
	return plane.a*vec.x + plane.b*vec.y + plane.c*vec.z + plane.d * 0;
}
WPlane WNormalizePlane(const WPlane plane) {
	WVector4 norm = WVec4Normalize(WVector4(plane.a, plane.b, plane.c, plane.d));
	return WPlane(norm.x, norm.y, norm.z, norm.w);
}

WColor WColorLerp(const WColor c1, const WColor c2, const float fLerpVal) {
	return c1 + (c2 - c1) * fLerpVal;
}