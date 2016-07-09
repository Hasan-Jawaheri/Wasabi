#pragma once

#include "WPhysicsComponent.h"

#include <stdio.h>
#include <conio.h>

#pragma comment(lib, "legacy_stdio_definitions.lib")

//
//Havok headers
//
// base includes
#include <Common/Base/hkBase.h>
#include <Common/Base/System/hkBaseSystem.h>
#include <Common/Base/System/Error/hkDefaultError.h>
#include <Common/Base/Memory/System/Util/hkMemoryInitUtil.h>
#include <Common/Base/Memory/System/hkMemorySystem.h>
#include <Common/Base/Monitor/hkMonitorStream.h>
#include <Common/Base/Types/hkBaseTypes.h>
#include <Common/Base/Types/Geometry/hkStridedVertices.h>
#include <Common/Base/Types/Geometry/hkGeometry.h>

// Dynamics includes
#include <Physics/Collide/hkpCollide.h>
#include <Physics/Collide/Shape/Convex/hkpConvexShape.h>
#include <Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h>
#include <Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h>
#include <Physics/Collide/Shape/hkpShapeType.h>
#include <Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h>
#include <Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesConnectivity.h>
#include <Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesConnectivityUtil.h>
#include <Physics/Collide/Util/hkpTriangleCompressor.h>
#include <Physics/Collide/Agent/ConvexAgent/SphereBox/hkpSphereBoxAgent.h>
#include <Physics/Collide/Shape/Convex/Box/hkpBoxShape.h>
#include <Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h>
#include <Physics/Collide/Dispatch/hkpAgentRegisterUtil.h>
#include <Physics/Collide/Util/ShapeCutter/hkpShapeCutterUtil.h>
#include <Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h>
#include <Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h>
#include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h>
#include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppUtility.h>
#include <Physics/Collide/Util/hkpTriangleUtil.h>
#include <Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifier.h>
#include <Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h>
#include <Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldBaseCinfo.h>
#include <Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h>
#include <Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h>
#include <Physics/Collide/Query/Collector/RayCollector/hkpClosestRayHitCollector.h>

#include <Physics/Dynamics/hkpDynamics.h>
#include <Physics/Dynamics/World/hkpWorld.h>
#include <Physics/Dynamics/Entity/hkpRigidBody.h>

#include <Physics/Dynamics/Constraint/hkpConstraintData.h>
#include <Physics/Dynamics/Constraint/hkpConstraintInstance.h>
#include <Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h>
#include <Physics/ConstraintSolver/Constraint/Bilateral/hkpInternalConstraintUtils.h>
#include <Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h>
#include <Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h>
#include <Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h>
#include <Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h>
#include <Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h>
#include <Physics/Dynamics/Constraint/Pulley/hkpPulleyConstraintData.h>
#include <Physics/ConstraintSolver/Constraint/Motor/hkpMotorConstraintInfo.h>
#include <Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h>
#include <Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h>
#include <Physics/Dynamics/Constraint/Motor/Callback/hkpCallbackConstraintMotor.h>
#include <Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h>
#include <Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h>
#include <Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h>
#include <Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h>
#include <Physics/Dynamics/Entity/hkpEntityListener.h>
#include <Physics/Dynamics/Collide/ContactListener/hkpContactListener.h>
#include <Physics/Dynamics/Action/hkpUnaryAction.h>

#include <Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h>

#include <Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h>
#include <Physics/Utilities/Collide/ShapeUtils/MoppCodeStreamer/hkpMoppCodeStreamer.h>
#include <Common/Base/System/Io/OArchive/hkOArchive.h>
#include <Common/Base/System/Io/IArchive/hkIArchive.h>

#include <Common/Base/Thread/Job/ThreadPool/Cpu/hkCpuJobThreadPool.h>
#include <Common/Base/Thread/Job/ThreadPool/Spu/hkSpuJobThreadPool.h>
#include <Common/Base/Thread/JobQueue/hkJobQueue.h>

// Visual Debugger includes
#include <Common/Visualize/hkVisualDebugger.h>
#include <Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpConstraintViewer.h>
#include <Physics/Utilities/VisualDebugger/hkpPhysicsContext.h>
#include <Physics/Utilities/VisualDebugger/Viewer/hkpShapeDisplayBuilder.h>

//key code
#include <Common/Base/KeyCode.h>


//
//Havok libraries
//

#pragma comment ( lib, "hkBase.lib" )
#pragma comment ( lib, "hkInternal.lib" )
#pragma comment ( lib, "hkGeometryUtilities.lib" )
#pragma comment ( lib, "hkVisualize.lib" )
#pragma comment ( lib, "hkSerialize.lib" )
#pragma comment ( lib, "hkSceneData.lib" )
#pragma comment ( lib, "hkCompat.lib" )

#pragma comment ( lib, "hkpConstraintSolver.lib" )
#pragma comment ( lib, "hkpCollide.lib" )
#pragma comment ( lib, "hkpDynamics.lib" )
#pragma comment ( lib, "hkpInternal.lib" )
#pragma comment ( lib, "hkpUtilities.lib" )

#pragma comment ( lib, "VdbClient.lib" )

//enums
enum W_MOTIONTYPE { MOTIONTYPE_DYNAMIC = 0, MOTIONTYPE_STATIC = 1, MOTIONTYPE_KEYFRAMED = 2 };
enum W_COLLISIONQUALITY {
	COLLISIONQUALITY_FIXED = 0, COLLISIONQUALITY_KEYFRAMED = 1,
	COLLISIONQUALITY_DEBRIS = 2, COLLISIONQUALITY_DEBRIS_SIMPLE_TOI = 3,
	COLLISIONQUALITY_MOVING = 4, COLLISIONQUALITY_CRITICAL = 5,
	COLLISIONQUALITY_BULLET = 6, COLLISIONQUALITY_USER = 7,
	COLLISIONQUALITY_CHARACTER = 8, COLLISIONQUALITY_KEYFRAMED_REPORTING = 9
};
enum W_RIGIDBODYTYPE { RIGIDBODY_BOX = 0, RIGIDBODY_SPHERE = 1, RIGIDBODY_COMPLEX = 2 };

//structures
struct _WRawPhysicsProps {
	float maxLinVel;
	hkVector4 linVel;
	float maxAngVel;
	hkVector4 angVel;
	float linDamp;
	float angDamp;
	W_MOTIONTYPE motionType;
	float bouncingPower;
	float mass;
	hkVector4 mc;
	float friction;
	hkpCollidableQualityType collQty;
};

/*********************************************************************
Special shapes
*********************************************************************/
/*********************************************************************
************************WHeightMapPhysicsShape***********************
This shape is a height map, such as a terrain
*********************************************************************/
class WHeightMapPhysicsShape : public hkpSampledHeightFieldShape {
public:
	WHeightMapPhysicsShape(const hkpSampledHeightFieldBaseCinfo& ci, float* data, bool bFlipped = false)
		: hkpSampledHeightFieldShape(ci),
		_data(data),
		_bFlipped(bFlipped) {
	}

	HK_FORCE_INLINE hkReal getHeightAtImpl(int x, int z) const {
		// Lookup data and return a float
		return hkReal(_data[(m_zRes - 1 - z) * m_zRes + x]);
	}

	//	This should return true if the two triangles share the edge p00-p11
	// otherwise it should return false if the triangles share the edge p01-p10
	HK_FORCE_INLINE hkBool getTriangleFlipImpl() const {
		return _bFlipped;
	}

	virtual void collideSpheres(const CollideSpheresInput& input, SphereCollisionOutput* outputArray) const {
		hkSampledHeightFieldShape_collideSpheres(*this, input, outputArray);
	}

private:

	float* _data;
	bool _bFlipped;
};

/*********************************************************************
******************************WRigidBody*****************************
Rigid body class
*********************************************************************/
class WRigidBody : public WBase, public WOrientation {
	std::string GetTypeName() const;

public:
	WRigidBody(class Wasabi* const app, unsigned int ID = 0);
	~WRigidBody();

	WError					BuildFromGeometries(W_RIGIDBODYTYPE type, WGeometry* const* geometries,
												unsigned int numGeometries, bool bFixed = false, bool bSimplify = true,
												void* mopp = nullptr, unsigned int moppsize = 0);
	void					Destroy();

	void					BindObject(WObject* const object);
	void					UnbindObject(WObject* const object);
	void					UnbindObject(unsigned int ID);

	void					Enable();
	void					Disable();
	bool					Enabled() const;

	void					SetLinearVelocity(float velX, float velY, float velZ);
	void					SetAngularVelocity(float velX, float velY, float velZ);
	void					SetLinearDamping(float power);
	void					SetAngularDamping(float power);
	void					SetMaxLinearVelocity(float fScalarVelocity);
	void					SetMaxAngularVelocity(float fScalarVelocity);
	void					SetMotionType(W_MOTIONTYPE type);
	void					SetBouncingPower(float bouncing);
	void					SetMass(float mass);
	void					SetMassCenter(float x, float y, float z);
	void					SetFriction(float friction);
	void					SetCollisionQuality(W_COLLISIONQUALITY quality);
	void					UpdateLocals();

	WMatrix					GetWorldMatrix();
	hkQuaternion			GetRotationQuat();
	W_RIGIDBODYTYPE			GetType() const;
	hkpRigidBody*			GetRigidBody() const;

	bool					Valid() const;
	void					OnStateChange(STATE_CHANGE_TYPE type);

private:
	bool				m_bAltered;
	WMatrix				m_WorldM;

	//physics
	hkpRigidBody*		m_rigidBody;
	W_RIGIDBODYTYPE		m_rbType;
	UINT				m_rbNumVerts;
	WVector3*			m_rbVerts;
	UINT				m_numGeometries;
	UINT*				m_geometryIDs;
	void*				m_moppcode;
	UINT				m_moppcodeSize;
	bool				m_physics;
	bool				m_simpleMesh;

	vector<WObject*>	m_objectV;
};

class WRigidBodyManager : public WManager<WRigidBody> {
	std::string GetTypeName() const;

public:
	WRigidBodyManager(Wasabi* const app);

	void Init();
	void SyncWithObjects();
	void Update(float fDeltaTime);

	void EnableMoppSaving();
	void DisableMoppSaving();
	bool IsMoppSavingEnabled() const;

private:
	bool m_bMoppSaving;
};

/*********************************************************************
******************************WConstraint****************************
Constraints abstract base class
*********************************************************************/
class WConstraint : public WBase{
	std::string GetTypeName() const;

public:
	WConstraint(class Wasabi* const app, unsigned int ID = 0);
	~WConstraint();

	virtual void	Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable = false) = 0;

	void				SetBreakingThreshold(float threshold);
	void				SetMaxLinearImpulse(float impulse);
	hkpConstraintData*	GetConstraint() const;

	bool				Valid() const;

protected:
	hkpRigidBody*			m_rb1;
	hkpRigidBody*			m_rb2;
	hkpWorld*				m_world;
	hkpConstraintData*		m_constraint;
	hkpConstraintInstance*	m_instance;
	hkpBreakableConstraintData* m_breakableConstraint;
};

class WConstraintManager : public WManager<WConstraint> {
	std::string GetTypeName() const;

public:
	WConstraintManager(Wasabi* const app);
};

/*********************************************************************
********************************WHinge*******************************
Constraints derived class that stands for hinge simulation, this is
mostly used for doors and such objects to simulate opening and closing
*********************************************************************/
class WHinge : public WConstraint {
public:
	WHinge(class Wasabi* const app, unsigned int ID = 0);
	~WHinge();

	void	Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable = false);

	void	SetPositionWorld(float x, float y, float z, float axisX, float axisY, float axisZ);
	void	SetPositionWorld(WVector3 position, WVector3 axis);
	void	SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2,
							 float axisX1, float axisY1, float axisZ1, float axisX2, float axisY2, float axisZ2);
	void	SetPositionLocal(WVector3 position1, WVector3 position2, WVector3 axis1, WVector3 axis2);

protected:
	hkpHingeConstraintData* m_hc;
};

/*********************************************************************
****************************WLimitedHinge****************************
Constraints derived class that stands for hinge simulation, but with
limited angle, this is mostly used for doors and such objects to
simulate opening and closing
*********************************************************************/
class WLimitedHinge : public WConstraint {
public:
	WLimitedHinge(class Wasabi* const app, unsigned int ID = 0);
	~WLimitedHinge();

	void	Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable = false);

	void	SetPositionWorld(float x, float y, float z, float axisX, float axisY, float axisZ);
	void	SetPositionWorld(WVector3 position, WVector3 axis);
	void	SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2,
							float axisX1, float axisY1, float axisZ1, float axisX2, float axisY2, float axisZ2,
							float axisX1perp, float axisY1perp, float axisZ1perp,
							float axisX2perp, float axisY2perp, float axisZ2perp);
	void	SetPositionLocal(WVector3 position1, WVector3 position2,
							 WVector3 axis1, WVector3 axis2, WVector3 axis1perp, WVector3 axis2perp);

	void	SetMaximumAngle(float angle);
	void	SetMinimumAngle(float angle);
	void	SetTauFactor(float factor);
	void	SetMaxFrictionTorque(float torque);

	void	SetMotor(void(__cdecl *cb)(const class hkpCallbackConstraintMotor& motor,
					const class hkpConstraintMotorInput* input, class hkpConstraintMotorOutput* output),
					float fMinForce, float fMaxForce);
	void	EnableMotor();
	void	DisableMotor();
	void	SetMotorTargetAngle(float ang);

protected:
	hkpLimitedHingeConstraintData* m_lhc;
};

/*********************************************************************
****************************WBallAndSocket***************************
Constraints derived class that stands for ball & socket simulation,
this is also known as a "point to point" constraint, and acts as if
two objects are joined at a point
*********************************************************************/
class WBallAndSocket : public WConstraint {
public:
	WBallAndSocket(class Wasabi* const app, unsigned int ID = 0);
	~WBallAndSocket();

	void	Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable = false);

	void	SetPositionWorld(float x, float y, float z);
	void	SetPositionWorld(WVector3 position);
	void	SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2);
	void	SetPositionLocal(WVector3 position1, WVector3 position2);

protected:
	hkpBallAndSocketConstraintData* m_bs;
};

/*********************************************************************
*****************************WStiffSpring****************************
Constraints derived class that stands for a stiff spring simulation,
this constraint acts as a rope between two objects
*********************************************************************/
class WStiffSpring : public WConstraint {
public:
	WStiffSpring(class Wasabi* const app, unsigned int ID = 0);
	~WStiffSpring();

	void	Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable = false);

	void	SetPositionWorld(float x1, float y1, float z1, float x2, float y2, float z2);
	void	SetPositionWorld(WVector3 position1, WVector3 position2);
	void	SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2, float length);
	void	SetPositionLocal(WVector3 position1, WVector3 position2, float length);

protected:
	hkpStiffSpringConstraintData* m_ss;
};

/*********************************************************************
********************************WPulley******************************
Constraints derived class that stands for a pulley simulation, this
constraint acts as a rope between two objects, but has two "pivots"
in space so it looks like a hoisting machine
*********************************************************************/
class WPulley : public WConstraint {
public:
	WPulley(class Wasabi* const app, unsigned int ID = 0);
	~WPulley();

	void	Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable = false);

	void	SetPositionWorld(float x, float y, float z, float x2, float y2, float z2,
							 float px, float py, float pz, float px2, float py2, float pz2);
	void	SetPositionWorld(WVector3 position1, WVector3 position2, WVector3 pivot1, WVector3 pivot2);
	void	SetPositionLocal(float x, float y, float z, float x2, float y2, float z2,
							 float px, float py, float pz, float px2, float py2, float pz2);
	void	SetPositionLocal(WVector3 position1, WVector3 position2, WVector3 pivot1, WVector3 pivot2);

	void	SetRopeLength(float length);
	void	SetLeverageOnB(float leverage);

protected:
	hkpPulleyConstraintData* m_pu;
};

/*********************************************************************
**************************WPrismaticConstraint***********************
Constraints derived class that stands for a prismatic constraint
simulation, this constraint acts as a slider, the object is limited
to 1 axis to move along
*********************************************************************/
class WPrismaticConstraint : public WConstraint {
public:
	WPrismaticConstraint(class Wasabi* const app, unsigned int ID = 0);
	~WPrismaticConstraint();

	void	Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable = false);

	void	SetPositionWorld(float x, float y, float z, float axisX, float axisY, float axisZ);
	void	SetPositionWorld(WVector3 position, WVector3 axis);
	void	SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2,
							float axisX1, float axisY1, float axisZ1, float axisX2, float axisY2, float axisZ2,
							float axisX1perp, float axisY1perp, float axisZ1perp,
							float axisX2perp, float axisY2perp, float axisZ2perp);
	void	SetPositionLocal(WVector3 position1, WVector3 position2,
		WVector3 axis1, WVector3 axis2, WVector3 axis1perp, WVector3 axis2perp);

	void	SetMaximumPosition(float pos);
	void	SetMinimumPosition(float pos);
	void	SetMaxFriction(float friction);

	void	SetMotor(void(__cdecl *cb)(const class hkpCallbackConstraintMotor& motor,
					const class hkpConstraintMotorInput* input, class hkpConstraintMotorOutput* output),
					float fMinForce, float fMaxForce);
	void	EnableMotor();
	void	DisableMotor();
	void	SetMotorTargetPos(float pos);

protected:
	hkpPrismaticConstraintData* m_pr;
};

class WPhysicsAction : public WBase, public hkpUnaryAction {
	std::string GetTypeName() const;

public:
	WPhysicsAction(class Wasabi* const app, WRigidBody* const body, unsigned int ID = 0);
	~WPhysicsAction();

	void Initialize();
	void Update(float fDeltaTime);

	virtual void applyAction(const hkStepInfo& stepInfo);

	virtual hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const {
		return HK_NULL;
	}

	virtual void SetMotionDirection(WVector3 dir);
	virtual WVector3 GetMotionDirection() const;

	bool Valid() const;

protected:
	WRigidBody*	m_rb;
	bool		m_valid;
};

class WPhysicsActionManager : public WManager<WPhysicsAction> {
	std::string GetTypeName() const;

public:
	WPhysicsActionManager(Wasabi* const app);

	void Update(float fDeltaTime);
};

class WMarbleAction : public WPhysicsAction {
public:
	WMarbleAction(class Wasabi* const app, WRigidBody* const body, unsigned int ID = 0);
	~WMarbleAction();

	void applyAction(const hkStepInfo& stepInfo);

	void SetForwardPressed(bool pressed);
	void SetBackwardPressed(bool pressed);
	void SetTurnLeftPressed(bool pressed);
	void SetTurnRightPressed(bool pressed);
	void SetJumpPressed(bool pressed);
	void SetBrakePressed(bool pressed);
	void SetStopPressed(bool pressed);
	void SetJumpingInfo(float fMultiplier, float fMarbleRadius);
	void SetRotationInfo(float fMultiplier);
	void SetAccelerationInfo(float fMultiplier);
	bool IsOnGround() const;

	void SetMotionDirection(WVector3 dir);
	WVector3 GetMotionDirection() const;

protected:
	float m_impulseScale;
	float m_rotationIncrement;
	float m_currentAngle;
	const hkVector4 m_forward;

	WVector3 m_curDir;
	float m_jumpMul;
	float m_fRadius;
	bool m_bForward;
	bool m_bBackward;
	bool m_bLeft;
	bool m_bRight;
	bool m_bJump;
	bool m_bBrake;
	bool m_bStop;
	bool m_bOnGround;
};

class WHavokPhysics : public WPhysicsComponent {
public:
	WHavokPhysics(class Wasabi* app);
	~WHavokPhysics();

	WRigidBodyManager*		RigidBodyManager;
	WConstraintManager*		ConstraintManager;
	WPhysicsActionManager*	ActionManager;

	virtual WError		Initialize();
	virtual void		Cleanup();
	virtual void		Start();
	virtual void		Stop();
	virtual void		Step(float deltaTime);
	virtual bool		Stepping() const;
	virtual bool		RayCast(WVector3 from, WVector3 to);
	virtual bool		RayCast(WVector3 from, WVector3 to, W_RAYCAST_OUTPUT* out);

	void				SetSpeed(float fSpeed);
	void				SetGravity(float x, float y, float z);
	void				SetGravity(WVector3 gravity);
	hkpWorld*			GetWorld() const;

private:
	bool						m_PHYSICS;
	bool						m_Initialized;
	bool						m_bVDB;
	bool						m_bStepping;
	float						m_speed;
	float						m_fPrevDeltaTimes[5];
	hkMemoryRouter				m_tmemoryRouter;
	hkpWorld*					m_physicsWorld;
	hkJobThreadPool*			m_threadPool;
	hkJobQueue*					m_jobQueue;
	hkpPhysicsContext*			m_context;
	hkVisualDebugger*			m_vdb;
};
