/** @file WMath.h
 *  @brief Basic math library
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include <cmath>
#include <cstdint>

#define W_EPSILON 0.0001f
#define W_PI (3.14159265358979323846f)
#define W_RADTODEG(rad) ((rad)*(180.0f/W_PI))
#define W_DEGTORAD(deg) ((deg)*(W_PI/180.0f))

/**
 * @ingroup engineclass
 *
 * Represents a color.
 */
class WColor {
public:
	/** red component of the color */
	float r;
	/** green component of the color */
	float g;
	/** blue component of the color */
	float b;
	/** alpha component of the color */
	float a;

	WColor() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
	WColor(uint32_t hex) : r((float)((hex >> 24) & 0xFF) / 255.0f), g((float)((hex >> 16) & 0xFF) / 255.0f), b((float)((hex >> 8) & 0xFF) / 255.0f), a((float)((hex) & 0xFF) / 255.0f) {}
	WColor(int hex) : r((float)(((uint32_t)hex >> 24) & 0xFF) / 255.0f), g((float)(((uint32_t)hex >> 16) & 0xFF) / 255.0f), b((float)(((uint32_t)hex >> 8) & 0xFF) / 255.0f), a((float)(((uint32_t)hex) & 0xFF) / 255.0f) {}
	WColor(float fRGB) : r(fRGB), g(fRGB), b(fRGB), a(1.0f) {}
	WColor(float fR, float fG, float fB) : r(fR), g(fG), b(fB), a(1.0f) {}
	WColor(float fR, float fG, float fB, float fA) : r(fR), g(fG), b(fB), a(fA){}

	operator float* () {
		return (float*)&r;
	}
	operator const float* () {
		return (const float*)&r;
	}

	const WColor operator+ () const {
		return *this;
	}
	const WColor operator- () const {
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

	float& operator[] (const uint32_t index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const uint32_t index) const {
		return *((float*)(this) + index);
	}
	float& operator[] (const int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const uint32_t index) {
		return *((float*)(this) + index);
	}
	const float operator() (const uint32_t index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const int index) const {
		return *((float*)(this) + index);
	}

	const bool operator== (const WColor col) const {
		return (fabs(r - col.r) < W_EPSILON && fabs(g - col.g) < W_EPSILON &&
			fabs(b - col.b) < W_EPSILON && fabs(a - col.a) < W_EPSILON);
	}
	const bool operator!= (const WColor col) const {
		return (fabs(r - col.r) >= W_EPSILON || fabs(g - col.g) >= W_EPSILON ||
			fabs(b - col.b) >= W_EPSILON || fabs(a - col.a) >= W_EPSILON);
	}
};

/**
 * @ingroup engineclass
 *
 * A 2D vector.
 */
class WVector2 {
public:
	/** x component of the vector */
	float x;
	/** y component of the vector */
	float y;

	WVector2() : x(0.0f), y(0.0f) {}
	WVector2(float fX, float fY) : x(fX), y(fY) {}

	operator float* () {
		return (float*)&x;
	}
	operator const float* () {
		return (const float*)&x;
	}

	const WVector2 operator+ () const {
		return *this;
	}
	const WVector2 operator- () const {
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

	float& operator[] (const uint32_t index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const uint32_t index) const {
		return *((float*)(this) + index);
	}
	float& operator[] (const int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const uint32_t index) {
		return *((float*)(this) + index);
	}
	const float operator() (const uint32_t index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const int index) const {
		return *((float*)(this) + index);
	}

	const bool operator== (const WVector2 vec) const {
		return (fabs(x - vec.x) < W_EPSILON && fabs(y - vec.y) < W_EPSILON);
	}
	const bool operator!= (const WVector2 vec) const {
		return (fabs(x - vec.x) >= W_EPSILON || fabs(y - vec.y) >= W_EPSILON);
	}
};

/**
 * @ingroup engineclass
 *
 * A 3D vector.
 */
class WVector3 {
public:
	/** x component of the vector */
	float x;
	/** y component of the vector */
	float y;
	/** z component of the vector */
	float z;

	WVector3() : x(0.0f), y(0.0f), z(0.0f) {}
	WVector3(float fX, float fY, float fZ) : x(fX), y(fY), z(fZ) {}

	operator float* () {
		return (float*)&x;
	}
	operator const float* () {
		return (const float*)&x;
	}

	const WVector3 operator+ () const {
		return *this;
	}
	const WVector3 operator- () const {
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

	float& operator[] (const uint32_t index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const uint32_t index) const {
		return *((float*)(this) + index);
	}
	float& operator[] (const int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const uint32_t index) {
		return *((float*)(this) + index);
	}
	const float operator() (const uint32_t index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const int index) const {
		return *((float*)(this) + index);
	}

	const bool operator== (const WVector3 vec) const {
		return (fabs(x - vec.x) < W_EPSILON && fabs(y - vec.y) <
						W_EPSILON && fabs(z - vec.z) < W_EPSILON);
	}
	const bool operator!= (const WVector3 vec) const {
		return (fabs(x - vec.x) >= W_EPSILON || fabs(y - vec.y) >=
						W_EPSILON || fabs(z - vec.z) >= W_EPSILON);
	}
};

/**
 * @ingroup engineclass
 *
 * A 4D vector.
 */
class WVector4 {
public:
	/** x component of the vector */
	float x;
	/** y component of the vector */
	float y;
	/** z component of the vector */
	float z;
	/** w component of the vector */
	float w;

	WVector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
	WVector4(float fX, float fY, float fZ) : x(fX), y(fY), z(fZ), w(0.0f) {}
	WVector4(float fX, float fY, float fZ, float fW)
		: x(fX), y(fY), z(fZ), w(fW) {}

	operator float* () {
		return (float*)&x;
	}
	operator const float* () {
		return (const float*)&x;
	}

	const WVector4 operator+ () const {
		return *this;
	}
	const WVector4 operator- () const {
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

	float& operator[] (const uint32_t index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const uint32_t index) const {
		return *((float*)(this) + index);
	}
	float& operator[] (const int index) {
		return *((float*)(this) + index);
	}
	const float operator[] (const int index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const uint32_t index) {
		return *((float*)(this) + index);
	}
	const float operator() (const uint32_t index) const {
		return *((float*)(this) + index);
	}
	float& operator() (const int index) {
		return *((float*)(this) + index);
	}
	const float operator() (const int index) const {
		return *((float*)(this) + index);
	}

	const bool operator== (const WVector4 vec) const {
		return (fabs(x - vec.x) < W_EPSILON && fabs(y - vec.y) < W_EPSILON &&
			fabs(z - vec.z) < W_EPSILON && fabs(w - vec.w) < W_EPSILON);
	}
	const bool operator!= (const WVector4 vec) const {
		return (fabs(x - vec.x) >= W_EPSILON || fabs(y - vec.y) >= W_EPSILON ||
			fabs(z - vec.z) >= W_EPSILON || fabs(w - vec.w) >= W_EPSILON);
	}
};


/**
 * @ingroup engineclass
 *
 * A quaternion.
 */
class WQuaternion : public WVector4 {
public:
	WQuaternion() : WVector4(0.0f, 0.0f, 0.0f, 1.0f) {}
	WQuaternion(float fX, float fY, float fZ, float fW)
		: WVector4(fX, fY, fZ, fW) {}
};

/**
 * @ingroup engineclass
 *
 * Represents a plane.
 */
class WPlane {
public:
	/** a component of the plane */
	float a;
	/** b component of the plane */
	float b;
	/** c component of the plane */
	float c;
	/** d component of the plane */
	float d;

	WPlane(void) : a(0), b(0), c(0), d(0) {}
	WPlane(float A, float B, float C, float D) : a(A), b(B), c(C), d(D) {}
};

/**
 * @ingroup engineclass
 *
 * A 4D matrix.
 */
class WMatrix {
public:
	/** Matrix components (row-major) */
	float mat[16];

	WMatrix();
	WMatrix(float f11, float f12, float f13, float f14,
		float f21, float f22, float f23, float f24,
		float f31, float f32, float f33, float f34,
		float f41, float f42, float f43, float f44);

	operator float* () {
		return (float*)&mat;
	}
	operator const float* () {
		return (const float*)&mat;
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

	float& operator() (const uint32_t row, const uint32_t col);
	const float operator() (const uint32_t row,const uint32_t col) const;
	float& operator[] (const uint32_t index);
	const float operator[] (const uint32_t index) const;
};

/**
 * Transposes a matrix.
 * @param  m matrix to transpose
 * @return   The transposed matrix
 */
const WMatrix WMatrixTranspose(const WMatrix m);

/**
 * Invert a matrix.
 * @param  m Matrix to invert
 * @return   Inverse of m
 */
const WMatrix WMatrixInverse(const WMatrix m);

/**
 * Dot product of two 2D vectors.
 * @param  v1 First vector
 * @param  v2 Second vector
 * @return    The dot product of v1 and v2
 */
const float WVec2Dot(const WVector2 v1, const WVector2 v2);

/**
 * Normalizes a 2D vector.
 * @param  v Vector to normalize
 * @return   Normalized vector
 */
const WVector2 WVec2Normalize(const WVector2 v);

/**
 * Calculates the length of a 2D vector.
 * @param  v Vector to calculate its length
 * @return   Length of v
 */
const float WVec2Length(const WVector2 v);

/**
 * Calculates the squared length of a 2D vector.
 * @param  v Vector to calculate its length
 * @return   Squared length of v
 */
const float WVec2LengthSq(const WVector2 v);

/**
 * Linearly interpolate two 2D vectors.
 * @param  v1       First vector
 * @param  v2       Second vector
 * @param  fLerpVal Linear interpolation value
 * @return          The interpolated vector
 */
const WVector2 WVec2Lerp(const WVector2 v1, const WVector2 v2,
												 const float fLerpVal);

/**
 * Transform a 2D vector by a matrix. This is the same as
 * WVec4Transform(WVector4(v.x, v.y, 0.0, 1.0), m).
 * @param  v Vector to transform
 * @param  m Matrix to transform by
 * @return   The result of the transformation
 */
const WVector4 WVec2Transform(const WVector2 v, const WMatrix m);

/**
 * Transform a 2D point by a matrix. This is the same as
 * WVec4Transform(WVector4(v.x, v.y, 0.0, 1.0), m), returning only the first
 * two components and dividing by the fourth component.
 * @param  v Vector to transform
 * @param  m Matrix to transform by
 * @return   The result of the transformation
 */
const WVector2 WVec2TransformCoord(const WVector2 v, const WMatrix m);

/**
 * Transform a 2D normal by a matrix. This is the same as
 * WVec4Transform(WVector4(v.x, v.y, 0.0, 0.0), m), returning only the first
 * two components.
 * @param  v Vector to transform
 * @param  m Matrix to transform by
 * @return   The result of the transformation
 */
const WVector2 WVec2TransformNormal(const WVector2 v, const WMatrix m);

/**
 * Dot product of two 3D vectors.
 * @param  v1 First vector
 * @param  v2 Second vector
 * @return    The dot product of v1 and v2
 */
const float WVec3Dot(const WVector3 v1, const WVector3 v2);

/**
 * Calculates the cross product of two 3D vectors.
 * @param  v1 First vector
 * @param  v2 Second vector
 * @return    Cross product of v1 and v2
 */
const WVector3 WVec3Cross(const WVector3 v1, const WVector3 v2);

/**
 * Normalizes a 3D vector.
 * @param  v Vector to normalize
 * @return   Normalized vector
 */
const WVector3 WVec3Normalize(const WVector3 v);

/**
 * Calculates the length of a 3D vector.
 * @param  v Vector to calculate its length
 * @return   Length of v
 */
const float WVec3Length(const WVector3 v);

/**
 * Calculates the squared length of a 3D vector.
 * @param  v Vector to calculate its length
 * @return   Squared length of v
 */
const float WVec3LengthSq(const WVector3 v);

/**
 * Linearly interpolate two 3D vectors.
 * @param  v1       First vector
 * @param  v2       Second vector
 * @param  fLerpVal Linear interpolation value
 * @return          The interpolated vector
 */
const WVector3 WVec3Lerp(const WVector3 v1, const WVector3 v2,
												 const float fLerpVal);

/**
 * Transform a 3D vector by a matrix. This is the same as
 * WVec4Transform(WVector4(v.x, v.y, v.z, 1.0), m).
 * @param  v Vector to transform
 * @param  m Matrix to transform by
 * @return   The result of the transformation
 */
const WVector4 WVec3Transform(const WVector3 v, const WMatrix m);

/**
 * Transform a 3D point by a matrix. This is the same as
 * WVec4Transform(WVector4(v.x, v.y, v.z, 1.0), m), returning only the first
 * three components and dividing by the fourth component.
 * @param  v Vector to transform
 * @param  m Matrix to transform by
 * @return   The result of the transformation
 */
const WVector3 WVec3TransformCoord(const WVector3 v, const WMatrix m);

/**
 * Transform a 3D normal by a matrix. This is the same as
 * WVec4Transform(WVector4(v.x, v.y, v.z, 0.0), m), returning only the first
 * three components.
 * @param  v Vector to transform
 * @param  m Matrix to transform by
 * @return   The result of the transformation
 */
const WVector3 WVec3TransformNormal(const WVector3 v, const WMatrix m);

/**
 * Dot product of two 4D vectors.
 * @param  v1 First vector
 * @param  v2 Second vector
 * @return    The dot product of v1 and v2
 */
const float WVec4Dot(const WVector4 v1, const WVector4 v2);

/**
 * Normalizes a 4D vector.
 * @param  v Vector to normalize
 * @return   Normalized vector
 */
const WVector4 WVec4Normalize(const WVector4 v);

/**
 * Calculates the length of a 4D vector.
 * @param  v Vector to calculate its length
 * @return   Length of v
 */
const float WVec4Length(const WVector4 v);

/**
 * Calculates the squared length of a 4D vector.
 * @param  v Vector to calculate its length
 * @return   Squared length of v
 */
const float WVec4LengthSq(const WVector4 v);

/**
 * Linearly interpolate two 4D vectors.
 * @param  v1       First vector
 * @param  v2       Second vector
 * @param  fLerpVal Linear interpolation value
 * @return          The interpolated vector
 */
const WVector4 WVec4Lerp(const WVector4 v1, const WVector4 v2, const float fLerpVal);

/**
 * Transform a 4D vector by a matrix.
 * @param  v Vector to transform
 * @param  m Matrix to transform by
 * @return   The result of the transformation
 */
const WVector4 WVec4Transform(const WVector4 v, const WMatrix m);

/**
 * Creates a matrix that performs a rotation on the X axis.
 * @param  fAngle Angle of rotation, in radians
 * @return        The resulting matrix
 */
const WMatrix WRotationMatrixX(const float fAngle);

/**
 * Creates a matrix that performs a rotation on the Y axis.
 * @param  fAngle Angle of rotation, in radians
 * @return        The resulting matrix
 */
const WMatrix WRotationMatrixY(const float fAngle);

/**
 * Creates a matrix that performs a rotation on the Z axis.
 * @param  fAngle Angle of rotation, in radians
 * @return        The resulting matrix
 */
const WMatrix WRotationMatrixZ(const float fAngle);

/**
 * Creates a matrix that performs a rotation on any given axis.
 * @param  axis   Axis of rotation
 * @param  fAngle Angle of rotation, in radians
 * @return        The resulting matrix
 */
const WMatrix WRotationMatrixAxis(const WVector3 axis, const float fAngle);

/**
 * Creates a matrix that performs scaling.
 * @param  fX Scaling on the X axis
 * @param  fY Scaling on the Y axis
 * @param  fZ Scaling on the Z axis
 * @return    The resulting matrix
 */
const WMatrix WScalingMatrix(const float fX, const float fY, const float fZ);

/**
 * Creates a matrix that performs scaling.
 * @param  scale Scaling components
 * @return       The resulting matrix
 */
const WMatrix WScalingMatrix(const WVector3 scale);

/**
 * Creates a matrix that performs translation.
 * @param  fX Translation on the X axis
 * @param  fY Translation on the Y axis
 * @param  fZ Translation on the Z axis
 * @return    The resulting matrix
 */
const WMatrix WTranslationMatrix(const float fX, const float fY,
																 const float fZ);

/**
 * Creates a matrix that performs translation.
 * @param  pos Translation components
 * @return     The resulting matrix
 */
const WMatrix WTranslationMatrix(const WVector3 pos);

/**
 * Creates a matrix that performs perspective projection.
 * @param  width  Width of the projection window
 * @param  height Height of the projection window
 * @param  znear  Near plane
 * @param  zfar   Far plane
 * @return        The resulting matrix
 */
const WMatrix WPerspectiveProjMatrix(const float width, const float height,
	const float znear, const float zfar);

/**
 * Creates a matrix that performs perspective projection.
 * @param  fFOV    Field of view, in radians
 * @param  fAspect Aspect ratio
 * @param  zn      Near plane
 * @param  zf      Far plane
 * @return         The resulting matrix
 */
const WMatrix WPerspectiveProjMatrixFOV(const float fFOV, const float fAspect,
	const float zn, const float zf);

/**
 * Creates a matrix that performs orthogonal projection.
 * @param  width  Width of the projection window
 * @param  height Height of the projection window
 * @param  znear  Near plane
 * @param  zfar   Far plane
 * @return        The resulting matrix
 */
const WMatrix WOrthogonalProjMatrix(const float width, const float height,
	const float znear, const float zfar);

/**
 * Perform a plane dot product with a point.
 * @param  plane Plane to perform the dot product
 * @param  vec   3D point
 * @return       Result of the dot product
 */
const float WPlaneDotCoord(const WPlane plane, const WVector3 vec);

/**
 * Perform a plane dot product with a normal.
 * @param  plane Plane to perform the dot product
 * @param  vec   3D normal vector
 * @return       Result of the dot product
 */
const float WPlaneDotNormal(const WPlane plane, const WVector3 vec);

/**
 * Normalizes a plane.
 * @param  plane Plane to normalize
 * @return       The resulting normalized plane
 */
const WPlane WNormalizePlane(const WPlane plane);

/**
 * Linearly interpolate two colors.
 * @param  v1       First color
 * @param  v2       Second color
 * @param  fLerpVal Linear interpolation value
 * @return          The interpolated color
 */
const WColor WColorLerp(const WColor c1, const WColor c2, const float fLerpVal);
