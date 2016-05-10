#include "WMath.h"
#include <memory>

WMatrix::WMatrix(void) {
	_11 = _22 = _33 = _44 = 1.0f;
	_12 = _13 = _14 = _21 = _23 = _24 = _31 = _32 = _34 = _41 = _42 = _43 = 0.0f;
}
WMatrix::WMatrix(float f11, float f12, float f13, float f14,
	float f21, float f22, float f23, float f24,
	float f31, float f32, float f33, float f34,
	float f41, float f42, float f43, float f44) :
	_11(f11), _12(f12), _13(f13), _14(f14),
	_21(f21), _22(f22), _23(f23), _24(f24),
	_31(f31), _32(f32), _33(f33), _34(f34),
	_41(f41), _42(f42), _43(f43), _44(f44) {
}
const WMatrix WMatrix::operator+ (const WMatrix m) const {
	WMatrix out = *this;
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			out(i, j) += m(i, j);
	return out;
}
const WMatrix WMatrix::operator- (const WMatrix m) const {
	WMatrix out = *this;
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			out(i, j) -= m(i, j);
	return out;
}
const WMatrix WMatrix::operator* (const WMatrix m) const {
	WMatrix out;
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++) {
			float fSum = 0.0f;
			for (unsigned int k = 0; k < 4; k++)
				fSum += (*this)(i, k) * m(k, j);
			out(i, j) = fSum;
		}
	return out;
}
const WMatrix WMatrix::operator/ (const WMatrix m) const {
	WMatrix out;
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++) {
			float fSum = 0.0f;
			for (unsigned int k = 0; k < 4; k++)
				fSum += (*this)(i, k) / m(k, j);
			out(i, j) = fSum;
		}
	return out;
}
const WMatrix WMatrix::operator* (const float f) const {
	WMatrix out = *this;
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			out(i, j) *= f;
	return out;
}
const WMatrix WMatrix::operator/ (const float f) const {
	WMatrix out = *this;
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			out(i, j) /= f;
	return out;
}
void WMatrix::operator+= (const WMatrix m) {
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			(*this)(i, j) += m(i, j);
}
void WMatrix::operator-= (const WMatrix m) {
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			(*this)(i, j) -= m(i, j);
}
void WMatrix::operator*= (const WMatrix m) {
	WMatrix old = (*this);
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++) {
			float fSum = 0.0f;
			for (unsigned int k = 0; k < 4; k++)
				fSum += old(i, k) * m(k, j);
			(*this)(i, j) = fSum;
		}
}
void WMatrix::operator/= (const WMatrix m) {
	WMatrix old;
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++) {
			float fSum = 0.0f;
			for (unsigned int k = 0; k < 4; k++)
				fSum += old(i, k) / m(k, j);
			(*this)(i, j) = fSum;
		}
}
void WMatrix::operator*= (const float f) {
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			(*this)(i, j) *= f;
}
void WMatrix::operator/= (const float f) {
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			(*this)(i, j) /= f;
}
float& WMatrix::operator() (const unsigned int row, const unsigned int col) {
	return (*this)[row * 4 + col];
}
const float WMatrix::operator() (const unsigned int row, const unsigned int col) const {
	return (*this)[row * 4 + col];
}
float& WMatrix::operator[] (const unsigned int index) {
	return *((float*)(this) + index);
}
const float WMatrix::operator[] (const unsigned int index) const {
	return *((float*)(this) + index);
}

const WMatrix WMatrixTranspose(const WMatrix m) {
	WMatrix out = m;

	out._12 = m._21;
	out._21 = m._12;
	out._13 = m._31;
	out._31 = m._13;
	out._14 = m._41;
	out._41 = m._14;
	out._23 = m._32;
	out._32 = m._23;
	out._24 = m._42;
	out._42 = m._24;
	out._34 = m._43;
	out._43 = m._34;

	return out;
}
const WMatrix WMatrixInverse(const WMatrix m) {
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

const float WVec2Dot(const WVector2 v1, const WVector2 v2) {
	return v1.x * v2.x + v1.y * v2.y;
}
const WVector2 WVec2Normalize(const WVector2 v) {
	return v / WVec2Length(v);
}
const float WVec2Length(const WVector2 v) {
	return sqrtf(v.x * v.x + v.y * v.y);
}
const float WVec2LengthSq(const WVector2 v) {
	return (v.x * v.x + v.y * v.y);
}
const WVector2 WVec2Lerp(const WVector2 v1, const WVector2 v2, const float fLerpVal) {
	return v1 + (v2 - v1) * fLerpVal;
}
const WVector4 WVec2Transform(const WVector2 v, const WMatrix m) {
	return WVec4Transform(WVector4(v.x, v.y, 0, 1), m);
}
const WVector2 WVec2TransformCoord(const WVector2 v, const WMatrix m) {
	WVector4 out = WVec4Transform(WVector4(v.x, v.y, 0, 1), m);
	return WVector2(out.x, out.y) / out.w;
}
const WVector2 WVec2TransformNormal(const WVector2 v, const WMatrix m) {
	WVector4 out = WVec4Transform(WVector4(v.x, v.y, 0, 0), m);
	return WVector2(out.x, out.y);
}

const float WVec3Dot(const WVector3 v1, const WVector3 v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
const WVector3 WVec3Cross(const WVector3 v1, const WVector3 v2) {
	WVector3 v;
	v.x = v1.y * v2.z - v1.z * v2.y;
	v.y = v1.z * v2.x - v1.x * v2.z;
	v.z = v1.x * v2.y - v1.y * v2.x;
	return v;
}
const WVector3 WVec3Normalize(const WVector3 v) {
	return v / WVec3Length(v);
}
const float WVec3Length(const WVector3 v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
const float WVec3LengthSq(const WVector3 v) {
	return (v.x * v.x + v.y * v.y + v.z * v.z);
}
const WVector3 WVec3Lerp(const WVector3 v1, const WVector3 v2, const float fLerpVal) {
	return v1 + (v2 - v1) * fLerpVal;
}
const WVector4 WVec3Transform(const WVector3 v, const WMatrix m) {
	return WVec4Transform(WVector4(v.x, v.y, v.z, 1), m);
}
const WVector3 WVec3TransformCoord(const WVector3 v, const WMatrix m) {
	WVector4 out = WVec4Transform(WVector4(v.x, v.y, v.z, 1), m);
	return WVector3(out.x, out.y, out.z) / out.w;
}
const WVector3 WVec3TransformNormal(const WVector3 v, const WMatrix m) {
	WVector4 out = WVec4Transform(WVector4(v.x, v.y, v.z, 0), m);
	return WVector3(out.x, out.y, out.z);
}

const float WVec4Dot(const WVector4 v1, const WVector4 v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}
const WVector4 WVec4Normalize(const WVector4 v) {
	return v / WVec4Length(v);
}
const float WVec4Length(const WVector4 v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}
const float WVec4LengthSq(const WVector4 v) {
	return (v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}
const WVector4 WVec4Lerp(const WVector4 v1, const WVector4 v2, const float fLerpVal) {
	return v1 + (v2 - v1) * fLerpVal;
}
const WVector4 WVec4Transform(const WVector4 v, const WMatrix m) {
	WVector4 out;
	out.x = v.x * m._11 + v.y * m._21 + v.z * m._31 + v.w * m._41;
	out.y = v.x * m._12 + v.y * m._22 + v.z * m._32 + v.w * m._42;
	out.z = v.x * m._13 + v.y * m._23 + v.z * m._33 + v.w * m._43;
	out.w = v.x * m._14 + v.y * m._24 + v.z * m._34 + v.w * m._44;
	return out;
}

const WMatrix WRotationMatrixX(const float fAngle) {
	return WMatrix(1.0f, 0, 0, 0,
		0, cosf(fAngle), sinf(fAngle), 0,
		0, -sinf(fAngle), cosf(fAngle), 0,
		0, 0, 0, 1.0f);
}
const WMatrix WRotationMatrixY(const float fAngle) {
	return WMatrix(cosf(fAngle), 0, -sinf(fAngle), 0,
		0, 1.0f, 0, 0,
		sinf(fAngle), 0, cosf(fAngle), 0,
		0, 0, 0, 1.0f);
}
const WMatrix WRotationMatrixZ(const float fAngle) {
	return WMatrix(cosf(fAngle), sinf(fAngle), 0, 0,
		-sinf(fAngle), cosf(fAngle), 0, 0,
		0, 0, 1.0f, 0,
		0, 0, 0, 1.0f);
}
const WMatrix WRotationMatrixAxis(const WVector3 axis, const float fAngle) {
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
const WMatrix WScalingMatrix(const float fX, const float fY, const float fZ) {
	WMatrix out = WMatrix();
	out._11 = fX;
	out._22 = fY;
	out._33 = fZ;
	return out;
}
const WMatrix WScalingMatrix(const WVector3 scale) {
	WMatrix out = WMatrix();
	out._11 = scale.x;
	out._22 = scale.y;
	out._33 = scale.z;
	return out;
}
const WMatrix WTranslationMatrix(const float fX, const float fY, const float fZ) {
	WMatrix out = WMatrix();
	out._41 = fX;
	out._42 = fY;
	out._43 = fZ;
	return out;
}
const WMatrix WTranslationMatrix(const WVector3 pos) {
	WMatrix out = WMatrix();
	out._41 = pos.x;
	out._42 = pos.y;
	out._43 = pos.z;
	return out;
}
const WMatrix WPerspectiveProjMatrix(const float w, const float h,
	const float zn, const float zf) {
	return WMatrix(2.0f*zn / w, 0, 0, 0,
		0, 2.0f*zn / h, 0, 0,
		0, 0, zf / (zf - zn), 1,
		0, 0, zn*zf / (zn - zf), 0);
}
const WMatrix WPerspectiveProjMatrixFOV(const float fFOV, const float fAspect,
	const float zn, const float zf) {
	float yScale = 1.0f / tanf(fFOV / 2.0f);
	float xScale = yScale / fAspect;
	return WMatrix(xScale, 0, 0, 0,
		0, yScale, 0, 0,
		0, 0, zf / (zf - zn), 1,
		0, 0, zn*zf / (zn - zf), 0);
}
const WMatrix WOrthogonalProjMatrix(const float w, const float h,
	const float zn, const float zf) {
	return WMatrix(2.0f / w, 0, 0, 0,
		0, 2.0f / h, 0, 0,
		0, 0, 1.0f / (zf - zn), 0,
		0, 0, zn / (zn - zf), 1);

}
const float WPlaneDotCoord(const WPlane plane, const WVector3 vec) {
	return plane.a*vec.x + plane.b*vec.y + plane.c*vec.z + plane.d * 1;
}
const float WPlaneDotNormal(const WPlane plane, const WVector3 vec) {
	return plane.a*vec.x + plane.b*vec.y + plane.c*vec.z + plane.d * 0;
}
const WPlane WNormalizePlane(const WPlane plane) {
	WVector4 norm = WVec4Normalize(WVector4(plane.a, plane.b, plane.c, plane.d));
	return WPlane(norm.x, norm.y, norm.z, norm.w);
}