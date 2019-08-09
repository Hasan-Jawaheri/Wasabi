#include "Physics/WRigidBody.h"
#include "Objects/WObject.h"
#include "Geometries/WGeometry.h"
#include "Core/WOrientation.h"

W_RIGID_BODY_CREATE_INFO W_RIGID_BODY_CREATE_INFO::ForComplexObject(WObject* object, bool isTriangleList) {
	W_RIGID_BODY_CREATE_INFO info = {};
	info.mass = 0.0f;
	info.shape = RIGID_BODY_SHAPE_MESH;
	info.isTriangleList = isTriangleList;
	info.geometry = object->GetGeometry();
	info.orientation = object;
	return info;
}

W_RIGID_BODY_CREATE_INFO W_RIGID_BODY_CREATE_INFO::ForComplexGeometry(WGeometry* geometry, bool isTriangleList, class WOrientation* orientation, WVector3 pos, WQuaternion rot) {
	W_RIGID_BODY_CREATE_INFO info = {};
	info.mass = 0.0f;
	info.shape = RIGID_BODY_SHAPE_MESH;
	info.isTriangleList = isTriangleList;
	info.geometry = geometry;
	info.orientation = orientation;
	info.initialPosition = pos;
	info.initialRotation = rot;
	return info;
}

W_RIGID_BODY_CREATE_INFO W_RIGID_BODY_CREATE_INFO::ForObject(WObject* object, float mass) {
	W_RIGID_BODY_CREATE_INFO info = {};
	info.mass = mass;
	info.shape = RIGID_BODY_SHAPE_CONVEX;
	info.geometry = object->GetGeometry();
	info.orientation = object;
	return info;
}

W_RIGID_BODY_CREATE_INFO W_RIGID_BODY_CREATE_INFO::ForGeometry(WGeometry* geometry, float mass, WOrientation* orientation, WVector3 pos, WQuaternion rot) {
	W_RIGID_BODY_CREATE_INFO info = {};
	info.mass = mass;
	info.shape = RIGID_BODY_SHAPE_CONVEX;
	info.geometry = geometry;
	info.orientation = orientation;
	info.initialPosition = pos;
	info.initialRotation = rot;
	return info;
}

W_RIGID_BODY_CREATE_INFO W_RIGID_BODY_CREATE_INFO::ForCube(WVector3 dimensions, float mass, WOrientation* orientation, WVector3 pos, WQuaternion rot) {
	W_RIGID_BODY_CREATE_INFO info = {};
	info.mass = mass;
	info.shape = RIGID_BODY_SHAPE_CUBE;
	info.dimensions = dimensions;
	info.orientation = orientation;
	info.initialPosition = pos;
	info.initialRotation = rot;
	return info;
}

W_RIGID_BODY_CREATE_INFO W_RIGID_BODY_CREATE_INFO::ForSphere(float radius, float mass, WOrientation* orientation, WVector3 pos, WQuaternion rot) {
	W_RIGID_BODY_CREATE_INFO info = {};
	info.mass = mass;
	info.shape = RIGID_BODY_SHAPE_SPHERE;
	info.dimensions = WVector3(radius, radius, radius);
	info.orientation = orientation;
	info.initialPosition = pos;
	info.initialRotation = rot;
	return info;
}

W_RIGID_BODY_CREATE_INFO W_RIGID_BODY_CREATE_INFO::ForCapsule(float height, float radius, float mass, WOrientation* orientation, WVector3 pos, WQuaternion rot) {
	W_RIGID_BODY_CREATE_INFO info = {};
	info.mass = mass;
	info.shape = RIGID_BODY_SHAPE_CAPSULE;
	info.dimensions = WVector3(radius, height, radius);
	info.orientation = orientation;
	info.initialPosition = pos;
	info.initialRotation = rot;
	return info;
}

W_RIGID_BODY_CREATE_INFO W_RIGID_BODY_CREATE_INFO::ForCylinder(float height, float radius, float mass, WOrientation* orientation, WVector3 pos, WQuaternion rot) {
	W_RIGID_BODY_CREATE_INFO info = {};
	info.mass = mass;
	info.shape = RIGID_BODY_SHAPE_CYLINDER;
	info.dimensions = WVector3(radius, height, radius);
	info.orientation = orientation;
	info.initialPosition = pos;
	info.initialRotation = rot;
	return info;
}

W_RIGID_BODY_CREATE_INFO W_RIGID_BODY_CREATE_INFO::ForCone(float height, float radius, float mass, WOrientation* orientation, WVector3 pos, WQuaternion rot) {
	W_RIGID_BODY_CREATE_INFO info = {};
	info.mass = mass;
	info.shape = RIGID_BODY_SHAPE_CONE;
	info.dimensions = WVector3(radius, height, radius);
	info.orientation = orientation;
	info.initialPosition = pos;
	info.initialRotation = rot;
	return info;
}

