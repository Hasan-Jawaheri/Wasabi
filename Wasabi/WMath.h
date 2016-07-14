/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine math helper library.
*********************************************************************/

#pragma once

#include <cmath>

#define W_EPSILON 0.0001f
#define W_PI (3.14159265358979323846)
#define W_RADTODEG(rad) ((rad)*(180.0f/W_PI))
#define W_DEGTORAD(deg) ((deg)*(W_PI/180.0f))


class WColor {
public:
	float r, g, b, a;

	WColor(void) : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
	WColor(float fRGB) : r(fRGB), g(fRGB), b(fRGB), a(1.0f) {}
	WColor(float fR, float fG, float fB) : r(fR), g(fG), b(fB), a(1.0f) {}
	WColor(float fR, float fG, float fB, float fA) : r(fR), g(fG), b(fB), a(fA) {}

	operator float* (void) {
		return (float*)&r;
	}
	operator const float* (void) {
		return (const float*)&r;
	}

	const WColor operator+ (void) const {
		return *this;
	}
	const WColor operator- (void) const {
		return WColor(-r, -g, -b, -a);
	}

	const WColor operator+ (const WColor col) const {
		return WColor(r + col.r, g + col.g, b + col.b, a + col.a);
	}
	const WColor operator- (const WColor col) const {
		return WColor(r - col.r, g - col.g, b - col.b, a - col.a);
	}
	const WColor operator* (const WColor col) const {
		return WColor(r * col.r, g * col.g, b * col.b, a * col.a);
	}
	const WColor operator* (const float f) const {
		return WColor(r * f, g * f, b * f, a * f);
	}
	friend const WColor operator* (const float f, const WColor v) {
		return WColor(f * v.r, f * v.g, f * v.b, f * v.a);
	}
	const WColor operator/ (const WColor col) const {
		return WColor(r / col.r, g / col.g, b / col.b, a / col.a);
	}
	const WColor operator/ (const float f) const {
		return WColor(r / f, g / f, b / f, a / f);
	}
	friend const WColor operator/ (const float f, const WColor v) {
		return WColor(f / v.r, f / v.g, f / v.b, f / v.a);
	}

	void operator+= (const WColor col) {
		r += col.r;
		g += col.g;
		b += col.b;
		a += col.a;
	}
	void operator-= (const WColor col) {
		r -= col.r;
		g -= col.g;
		b -= col.b;
		a -= col.a;
	}
	void operator*= (const WColor col) {
		r *= col.r;
		g *= col.g;
		b *= col.b;
		a *= col.a;
	}
	void operator*= (const float f) {
		r *= f;
		g *= f;
		b *= f;
		a *= f;
	}
	void operator/= (const WColor col) {
		r /= col.r;
		g /= col.g;
		b /= col.b;
		a /= col.a;
	}
	void operator/= (const float f) {
		r /= f;
		g /= f;
		b /= f;
		a /= f;
	}

	float& operator[] (const unsigned int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const unsigned int index) const {
		return *((float*)(this) + index);
	}
	float& operator[] (const int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const unsigned int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const unsigned int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const int index) const {
		return *((float*)(this) + index);
	}

	const bool operator== (const WColor col) const {
		return (abs(r - col.r) < W_EPSILON && abs(g - col.g) < W_EPSILON &&
			abs(b - col.b) < W_EPSILON && abs(a - col.a) < W_EPSILON);
	}
	const bool operator!= (const WColor col) const {
		return (abs(r - col.r) >= W_EPSILON || abs(g - col.g) >= W_EPSILON ||
			abs(b - col.b) >= W_EPSILON || abs(a - col.a) >= W_EPSILON);
	}
};

class WVector2 {
public:
	float x, y;

	WVector2(void) : x(0.0f), y(0.0f) {}
	WVector2(float fX, float fY) : x(fX), y(fY) {}

	operator float* (void) {
		return (float*)&x;
	}
	operator const float* (void) {
		return (const float*)&x;
	}

	const WVector2 operator+ (void) const {
		return *this;
	}
	const WVector2 operator- (void) const {
		return WVector2(-x, -y);
	}

	const WVector2 operator+ (const WVector2 vec) const {
		return WVector2(x + vec.x, y + vec.y);
	}
	const WVector2 operator- (const WVector2 vec) const {
		return WVector2(x - vec.x, y - vec.y);
	}
	const WVector2 operator* (const WVector2 vec) const {
		return WVector2(x * vec.x, y * vec.y);
	}
	const WVector2 operator* (const float f) const {
		return WVector2(x * f, y * f);
	}
	friend const WVector2 operator* (const float f, const WVector2 v) {
		return WVector2(f * v.x, f * v.y);
	}
	const WVector2 operator/ (const WVector2 vec) const {
		return WVector2(x / vec.x, y / vec.y);
	}
	const WVector2 operator/ (const float f) const {
		return WVector2(x / f, y / f);
	}
	friend const WVector2 operator/ (const float f, const WVector2 v) {
		return WVector2(f / v.x, f / v.y);
	}

	void operator+= (const WVector2 vec) {
		x += vec.x;
		y += vec.y;
	}
	void operator-= (const WVector2 vec) {
		x -= vec.x;
		y -= vec.y;
	}
	void operator*= (const WVector2 vec) {
		x *= vec.x;
		y *= vec.y;
	}
	void operator*= (const float f) {
		x *= f;
		y *= f;
	}
	void operator/= (const WVector2 vec) {
		x /= vec.x;
		y /= vec.y;
	}
	void operator/= (const float f) {
		x /= f;
		y /= f;
	}

	float& operator[] (const unsigned int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const unsigned int index) const {
		return *((float*)(this) + index);
	}
	float& operator[] (const int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const unsigned int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const unsigned int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const int index) const {
		return *((float*)(this) + index);
	}

	const bool operator== (const WVector2 vec) const {
		return (abs(x - vec.x) < W_EPSILON && abs(y - vec.y) < W_EPSILON);
	}
	const bool operator!= (const WVector2 vec) const {
		return (abs(x - vec.x) >= W_EPSILON || abs(y - vec.y) >= W_EPSILON);
	}
};

class WVector3 {
public:
	float x, y, z;

	WVector3(void) : x(0.0f), y(0.0f), z(0.0f) {}
	WVector3(float fX, float fY, float fZ) : x(fX), y(fY), z(fZ) {}

	operator float* (void) {
		return (float*)&x;
	}
	operator const float* (void) {
		return (const float*)&x;
	}

	const WVector3 operator+ (void) const {
		return *this;
	}
	const WVector3 operator- (void) const {
		return WVector3(-x, -y, -z);
	}

	const WVector3 operator+ (const WVector3 vec) const {
		return WVector3(x + vec.x, y + vec.y, z + vec.z);
	}
	const WVector3 operator- (const WVector3 vec) const {
		return WVector3(x - vec.x, y - vec.y, z - vec.z);
	}
	const WVector3 operator* (const WVector3 vec) const {
		return WVector3(x * vec.x, y * vec.y, z * vec.z);
	}
	const WVector3 operator* (const float f) const {
		return WVector3(x * f, y * f, z * f);
	}
	friend const WVector3 operator* (const float f, const WVector3 v) {
		return WVector3(f * v.x, f * v.y, f * v.z);
	}
	const WVector3 operator/ (const WVector3 vec) const {
		return WVector3(x / vec.x, y / vec.y, z / vec.z);
	}
	const WVector3 operator/ (const float f) const {
		return WVector3(x / f, y / f, z / f);
	}
	friend const WVector3 operator/ (const float f, const WVector3 v) {
		return WVector3(f / v.x, f / v.y, f / v.z);
	}

	void operator+= (const WVector3 vec) {
		x += vec.x;
		y += vec.y;
		z += vec.z;
	}
	void operator-= (const WVector3 vec) {
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
	}
	void operator*= (const WVector3 vec) {
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
	}
	void operator*= (const float f) {
		x *= f;
		y *= f;
		z *= f;
	}
	void operator/= (const WVector3 vec) {
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
	}
	void operator/= (const float f) {
		x /= f;
		y /= f;
		z /= f;
	}

	float& operator[] (const unsigned int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const unsigned int index) const {
		return *((float*)(this) + index);
	}
	float& operator[] (const int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const unsigned int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const unsigned int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const int index) const {
		return *((float*)(this) + index);
	}

	const bool operator== (const WVector3 vec) const {
		return (abs(x - vec.x) < W_EPSILON && abs(y - vec.y) < W_EPSILON && abs(z - vec.z) < W_EPSILON);
	}
	const bool operator!= (const WVector3 vec) const {
		return (abs(x - vec.x) >= W_EPSILON || abs(y - vec.y) >= W_EPSILON || abs(z - vec.z) >= W_EPSILON);
	}
};

class WVector4 {
public:
	float x, y, z, w;

	WVector4(void) : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
	WVector4(float fX, float fY, float fZ) : x(fX), y(fY), z(fZ), w(0.0f) {}
	WVector4(float fX, float fY, float fZ, float fW) : x(fX), y(fY), z(fZ), w(fW) {}

	operator float* (void) {
		return (float*)&x;
	}
	operator const float* (void) {
		return (const float*)&x;
	}

	const WVector4 operator+ (void) const {
		return *this;
	}
	const WVector4 operator- (void) const {
		return WVector4(-x, -y, -z, -w);
	}

	const WVector4 operator+ (const WVector4 vec) const {
		return WVector4(x + vec.x, y + vec.y, z + vec.z, w + vec.w);
	}
	const WVector4 operator- (const WVector4 vec) const {
		return WVector4(x - vec.x, y - vec.y, z - vec.z, w - vec.w);
	}
	const WVector4 operator* (const WVector4 vec) const {
		return WVector4(x * vec.x, y * vec.y, z * vec.z, w * vec.w);
	}
	const WVector4 operator* (const float f) const {
		return WVector4(x * f, y * f, z * f, w * f);
	}
	friend const WVector4 operator* (const float f, const WVector4 v) {
		return WVector4(f * v.x, f * v.y, f * v.z, f * v.w);
	}
	const WVector4 operator/ (const WVector4 vec) const {
		return WVector4(x / vec.x, y / vec.y, z / vec.z, w / vec.w);
	}
	const WVector4 operator/ (const float f) const {
		return WVector4(x / f, y / f, z / f, w / f);
	}
	friend const WVector4 operator/ (const float f, const WVector4 v) {
		return WVector4(f / v.x, f / v.y, f / v.z, f / v.w);
	}

	void operator+= (const WVector4 vec) {
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
	}
	void operator-= (const WVector4 vec) {
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
	}
	void operator*= (const WVector4 vec) {
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		w *= vec.w;
	}
	void operator*= (const float f) {
		x *= f;
		y *= f;
		z *= f;
		w *= f;
	}
	void operator/= (const WVector4 vec) {
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		w /= vec.w;
	}
	void operator/= (const float f) {
		x /= f;
		y /= f;
		z /= f;
		w /= f;
	}

	float& operator[] (const unsigned int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const unsigned int index) const {
		return *((float*)(this) + index);
	}
	float& operator[] (const int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const unsigned int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const unsigned int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const int index) const {
		return *((float*)(this) + index);
	}

	const bool operator== (const WVector4 vec) const {
		return (abs(x - vec.x) < W_EPSILON && abs(y - vec.y) < W_EPSILON &&
			abs(z - vec.z) < W_EPSILON && abs(w - vec.w) < W_EPSILON);
	}
	const bool operator!= (const WVector4 vec) const {
		return (abs(x - vec.x) >= W_EPSILON || abs(y - vec.y) >= W_EPSILON ||
			abs(z - vec.z) >= W_EPSILON || abs(w - vec.w) >= W_EPSILON);
	}
};

class WPlane {
public:
	float a, b, c, d;

	WPlane(void) : a(0), b(0), c(0), d(0) {}
	WPlane(float A, float B, float C, float D) : a(A), b(B), c(C), d(D) {}
};

class WMatrix {
public:
	float _11, _12, _13, _14;
	float _21, _22, _23, _24;
	float _31, _32, _33, _34;
	float _41, _42, _43, _44;

	WMatrix(void);
	WMatrix(float f11, float f12, float f13, float f14,
		float f21, float f22, float f23, float f24,
		float f31, float f32, float f33, float f34,
		float f41, float f42, float f43, float f44);

	operator float* (void) {
		return (float*)&_11;
	}
	operator const float* (void) {
		return (const float*)&_11;
	}

	const WMatrix operator+ (const WMatrix m) const;
	const WMatrix operator- (const WMatrix m) const;
	const WMatrix operator* (const WMatrix m) const;
	const WMatrix operator/ (const WMatrix m) const;

	const WMatrix operator* (const float f) const;
	const WMatrix operator/ (const float f) const;

	void operator+= (const WMatrix m);
	void operator-= (const WMatrix m);
	void operator*= (const WMatrix m);
	void operator/= (const WMatrix m);

	void operator*= (const float f);
	void operator/= (const float f);

	float& operator() (const unsigned int row, const unsigned int col);
	const float operator() (const unsigned int row, const unsigned int col) const;
	float& operator[] (const unsigned int index);
	const float operator[] (const unsigned int index) const;
};

const WMatrix WMatrixTranspose(const WMatrix m);
const WMatrix WMatrixInverse(const WMatrix m);

const float WVec2Dot(const WVector2 v1, const WVector2 v2);
const WVector2 WVec2Normalize(const WVector2 v);
const float WVec2Length(const WVector2 v);
const float WVec2LengthSq(const WVector2 v);
const WVector2 WVec2Lerp(const WVector2 v1, const WVector2 v2, const float fLerpVal);
const WVector4 WVec2Transform(const WVector2 v, const WMatrix m);
const WVector2 WVec2TransformCoord(const WVector2 v, const WMatrix m);
const WVector2 WVec2TransformNormal(const WVector2 v, const WMatrix m);

const float WVec3Dot(const WVector3 v1, const WVector3 v2);
const WVector3 WVec3Cross(const WVector3 v1, const WVector3 v2);
const WVector3 WVec3Normalize(const WVector3 v);
const float WVec3Length(const WVector3 v);
const float WVec3LengthSq(const WVector3 v);
const WVector3 WVec3Lerp(const WVector3 v1, const WVector3 v2, const float fLerpVal);
const WVector4 WVec3Transform(const WVector3 v, const WMatrix m);
const WVector3 WVec3TransformCoord(const WVector3 v, const WMatrix m);
const WVector3 WVec3TransformNormal(const WVector3 v, const WMatrix m);

const float WVec4Dot(const WVector4 v1, const WVector4 v2);
const WVector4 WVec4Normalize(const WVector4 v);
const float WVec4Length(const WVector4 v);
const float WVec4LengthSq(const WVector4 v);
const WVector4 WVec4Lerp(const WVector4 v1, const WVector4 v2, const float fLerpVal);
const WVector4 WVec4Transform(const WVector4 v, const WMatrix m);

const WMatrix WRotationMatrixX(const float fAngle);
const WMatrix WRotationMatrixY(const float fAngle);
const WMatrix WRotationMatrixZ(const float fAngle);
const WMatrix WRotationMatrixAxis(const WVector3 axis, const float fAngle);
const WMatrix WScalingMatrix(const float fX, const float fY, const float fZ);
const WMatrix WScalingMatrix(const WVector3 scale);
const WMatrix WTranslationMatrix(const float fX, const float fY, const float fZ);
const WMatrix WTranslationMatrix(const WVector3 pos);
const WMatrix WPerspectiveProjMatrix(const float width, const float height,
	const float znear, const float zfar);
const WMatrix WPerspectiveProjMatrixFOV(const float fFOV, const float fAspect,
	const float zn, const float zf);
const WMatrix WOrthogonalProjMatrix(const float width, const float height,
	const float znear, const float zfar);

const float WPlaneDotCoord(const WPlane plane, const WVector3 vec);
const float WPlaneDotNormal(const WPlane plane, const WVector3 vec);
const WPlane WNormalizePlane(const WPlane plane);


typedef WVector4 WQuaternion;
