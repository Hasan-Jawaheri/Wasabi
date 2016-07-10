#include "WHavokPhysics.h"

#if 0

// Classlists
#define HK_CLASSES_FILE <Common/Serialize/Classlist/hkClasses.h>
#include <Common/Serialize/Util/hkBuiltinTypeRegistry.cxx>

#include <Common/Base/KeyCode.cxx>

//physics error reporting
static void HK_CALL errorReport(const char* msg, void* userArgGivenToInit) {
	std::cout << "Havok Physics Error: " << msg << std::endl;
}

WHavokPhysics::WHavokPhysics(Wasabi* app) : WPhysicsComponent(app) {
	m_PHYSICS = false;
	m_Initialized = false;
	m_bVDB = false;
	m_speed = 1.0f;
	m_bStepping = false;
	for (UINT i = 0; i < 5; i++)
		m_fPrevDeltaTimes[i] = 0.0f;

	RigidBodyManager = new WRigidBodyManager(m_app);
	ConstraintManager = new WConstraintManager(m_app);
	ActionManager = new WPhysicsActionManager(m_app);

	void* val;
	float fval = 1.0f;
	memcpy(&val, &fval, sizeof(float));
	m_app->engineParams.insert(std::pair<std::string, void*>("physicsSpeed", (void*)(val)));
	fval = 10000.0f;
	memcpy(&val, &fval, sizeof(float));
	m_app->engineParams.insert(std::pair<std::string, void*>("physicsWorldSize", (void*)(val)));
	m_app->engineParams.insert(std::pair<std::string, void*>("physicsDebugger", (void*)(false)));
	m_app->engineParams.insert(std::pair<std::string, void*>("physicsSolver",
		(void*)(hkpWorldCinfo::SOLVER_TYPE_4ITERS_MEDIUM)));
}

WHavokPhysics::~WHavokPhysics() {
	W_SAFE_DELETE(RigidBodyManager);
	W_SAFE_DELETE(ConstraintManager);
	W_SAFE_DELETE(ActionManager);

	Cleanup();
}

void WHavokPhysics::Cleanup() {
	if (m_Initialized) {
		m_physicsWorld->markForWrite();
		m_physicsWorld->removeReference();
		if (m_bVDB) {
			m_vdb->removeReference();
			m_context->removeReference();
		}

		W_SAFE_DELETE(m_jobQueue);

		m_threadPool->removeReference();

		m_Initialized = false;

		//un-initialize havok
		hkBaseSystem::quitThread();
		hkMemorySystem::getInstance().threadQuit(m_tmemoryRouter);

		//un-initialize havok
		hkBaseSystem::quit();
		hkMemoryInitUtil::quit();
	}
}

WError WHavokPhysics::Initialize() {
	Cleanup();

	//initialize havok
	hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initDefault();
	hkBaseSystem::init(memoryRouter, errorReport);

	//initialize havok
	hkMemorySystem::getInstance().threadInit(m_tmemoryRouter, "hkCpuJobThreadPool");
	hkBaseSystem::initThread(&m_tmemoryRouter);

	memcpy(&m_speed, &m_app->engineParams["physicsSpeed"], sizeof(float));
	float fWorldSize;
	memcpy(&fWorldSize, &m_app->engineParams["physicsWorldSize"], sizeof(float));

	int totalNumThreadsUsed;

	hkHardwareInfo hwInfo;
	hkGetHardwareInfo(hwInfo);
	totalNumThreadsUsed = hwInfo.m_numThreads;

	hkCpuJobThreadPoolCinfo threadPoolCinfo;
	threadPoolCinfo.m_numThreads = totalNumThreadsUsed - 1;

	threadPoolCinfo.m_timerBufferPerThreadAllocation = 200000;
	m_threadPool = new hkCpuJobThreadPool(threadPoolCinfo);

	hkJobQueueCinfo info;
	info.m_jobQueueHwSetup.m_numCpuThreads = totalNumThreadsUsed;
	m_jobQueue = new hkJobQueue(info);

	hkMonitorStream::getInstance().resize(200000);

	hkpWorldCinfo worldInfo;

	worldInfo.m_gravity = hkVector4(0, -9.8, 0);
	worldInfo.setBroadPhaseWorldSize(fWorldSize);
	worldInfo.m_broadPhaseBorderBehaviour = hkpWorldCinfo::BROADPHASE_BORDER_DO_NOTHING;
	worldInfo.m_simulationType = hkpWorldCinfo::SIMULATION_TYPE_MULTITHREADED;
	worldInfo.setupSolverInfo((hkpWorldCinfo::SolverType)(int)m_app->engineParams["physicsSolver"]);
	m_physicsWorld = new hkpWorld(worldInfo);
	m_physicsWorld->lock();
	hkpAgentRegisterUtil::registerAllAgents(m_physicsWorld->getCollisionDispatcher());
	m_physicsWorld->registerWithJobQueue(m_jobQueue);

	//visual debugger
	if (m_app->engineParams["physicsDebugger"]) {
		m_bVDB = true;

		hkArray<hkProcessContext*> contexts;

		m_context = new hkpPhysicsContext();
		hkpPhysicsContext::registerAllPhysicsProcesses();
		m_context->addWorld(m_physicsWorld);
		contexts.pushBack(m_context);

		m_vdb = new hkVisualDebugger(contexts);
		m_vdb->serve();
	}

	m_physicsWorld->unlock();

	m_Initialized = true;

	return WError(W_SUCCEEDED);
}

void WHavokPhysics::Start() {
	if (m_Initialized && !m_PHYSICS) {
		//start physics
		RigidBodyManager->SyncWithObjects();
		m_PHYSICS = true; //mark physics as started
	}
}

void WHavokPhysics::Stop() {
	m_PHYSICS = false; //mark physics as stopped
}

void WHavokPhysics::SetSpeed(float fSpeed) {
	m_speed = fSpeed;
}

void WHavokPhysics::SetGravity(float x, float y, float z) {
	m_physicsWorld->lock();
	m_physicsWorld->setGravity(hkVector4(x, y, z));
	m_physicsWorld->unlock();
}

void WHavokPhysics::SetGravity(WVector3 gravity) {
	m_physicsWorld->setGravity(hkVector4(gravity.x, gravity.y, gravity.z));
}

void WHavokPhysics::Step(float deltaTime) {
	if (m_PHYSICS && m_Initialized && deltaTime && deltaTime < 1) {
		m_bStepping = true;

		float finalDeltaTime = 0.0f; //take the average between the 5 steps to make it smoother
		for (UINT i = 0; i < 5; i++) {
			if (m_fPrevDeltaTimes[i] == 0) {
				m_fPrevDeltaTimes[i] = deltaTime;
				return;
			}
			finalDeltaTime += m_fPrevDeltaTimes[i];
		}
		finalDeltaTime /= 5.0f;

		ActionManager->Update(finalDeltaTime);

		m_physicsWorld->stepMultithreaded(m_jobQueue, m_threadPool, finalDeltaTime * m_speed);

		if (m_bVDB) {
			m_context->syncTimers(m_threadPool);
			m_vdb->step();
		}

		hkMonitorStream::getInstance().reset();
		m_threadPool->clearTimerData();

		RigidBodyManager->Update(deltaTime);

		//update delta times
		for (UINT i = 1; i < 5; i++)
			m_fPrevDeltaTimes[i - 1] = m_fPrevDeltaTimes[i];
		m_fPrevDeltaTimes[4] = deltaTime;

		m_bStepping = false;
	}
}

hkpWorld* WHavokPhysics::GetWorld() const {
	if (m_Initialized)
		return m_physicsWorld;
	else
		return nullptr;
}

bool WHavokPhysics::Stepping() const {
	return m_bStepping;
}

bool WHavokPhysics::RayCast(WVector3 from, WVector3 to) {
	m_physicsWorld->lock();
	hkpWorldRayCastInput input;
	input.m_from.set(from.x, from.y, from.z);
	input.m_to.set(to.x, to.y, to.z);
	hkpClosestRayHitCollector output;
	m_physicsWorld->castRay(input, output);
	m_physicsWorld->unlock();

	return output.hasHit();
}

bool WHavokPhysics::RayCast(WVector3 from, WVector3 to, W_RAYCAST_OUTPUT* out) {
	m_physicsWorld->lock();
	hkpWorldRayCastInput input;
	input.m_from.set(from.x, from.y, from.z);
	input.m_to.set(to.x, to.y, to.z);
	hkpClosestRayHitCollector output;
	m_physicsWorld->castRay(input, output);
	m_physicsWorld->unlock();

	if (out) {
		hkVector4 n = output.getHit().m_normal;
		out->normal = WVector3(n(0), n(1), n(2));
		out->fDist = WVec3Length(from - to) * output.getHit().m_hitFraction;
	}

	return output.hasHit();
}

WRigidBodyManager::WRigidBodyManager(Wasabi* app) : WManager<WRigidBody>(m_app) {
	m_bMoppSaving = false;
}

void WRigidBodyManager::SyncWithObjects() {
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	for (UINT j = 0; j < W_HASHTABLESIZE; j++) {
		for (UINT i = 0; i < m_entities[j].size(); i++) {
			if (!m_entities[j][i]->GetRigidBody())
				continue;

			//tell havok the current object's position/rotation before starting to simulate physics
			m_entities[j][i]->GetRigidBody()->setPosition(hkVector4(m_entities[j][i]->GetPosition().x,
																	m_entities[j][i]->GetPosition().y, m_entities[j][i]->GetPosition().z));
			m_entities[j][i]->GetRigidBody()->setRotation(m_entities[j][i]->GetRotationQuat());
		}
	}
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBodyManager::Update(float fDeltaTime) {
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	for (UINT j = 0; j < W_HASHTABLESIZE; j++) {
		for (UINT i = 0; i < m_entities[j].size(); i++) {
			if (m_entities[j][i]->GetRigidBody() && m_entities[j][i]->Enabled()) {
				hkVector4 v = m_entities[j][i]->GetRigidBody()->getPosition(); //get position
				hkQuaternion q = m_entities[j][i]->GetRigidBody()->getRotation(); //get rotation

				m_entities[j][i]->SetPosition(v(0), v(1), v(2));

				WQuaternion quat(q(0), q(1), q(2), q(3));
				//re-build the ulr vectors of the object
				m_entities[j][i]->SetAngle(quat);
			}
		}
	}
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBodyManager::Init() {
}

void WRigidBodyManager::EnableMoppSaving() {
	m_bMoppSaving = true;
}

void WRigidBodyManager::DisableMoppSaving() {
	m_bMoppSaving = false;
}

bool WRigidBodyManager::IsMoppSavingEnabled() const {
	return m_bMoppSaving;
}

std::string WRigidBodyManager::GetTypeName() const {
	return "RigidBody";
}

//rigid body class implemention
WRigidBody::WRigidBody(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_physics = true;
	m_rigidBody = nullptr;
	m_rbVerts = nullptr;
	m_rbNumVerts = 0;
	m_simpleMesh = true;
	m_numGeometries = 0;
	m_geometryIDs = nullptr;
	m_moppcode = nullptr;
	m_moppcodeSize = 0;

	//register the object
	((WHavokPhysics*)m_app->PhysicsComponent)->RigidBodyManager->AddEntity(this);
}
WRigidBody::~WRigidBody() {
	Destroy();

	//unregister object
	((WHavokPhysics*)m_app->PhysicsComponent)->RigidBodyManager->RemoveEntity(this);
}

std::string WRigidBody::GetTypeName() const {
	return "Rigid Body";
}

void WRigidBody::BindObject(WObject* const object) {
	if (object) {
		object->AddReference();
		m_objectV.push_back(object);
	}
}

void WRigidBody::UnbindObject(WObject* const object) {
	for (UINT i = 0; i < m_objectV.size(); i++)
		if (m_objectV[i] == object) {
			W_SAFE_REMOVEREF(m_objectV[i]);
			m_objectV.erase(m_objectV.begin() + i);
			break;
		}
}

void WRigidBody::UnbindObject(UINT ID) {
	for (UINT i = 0; i < m_objectV.size(); i++)
		if (m_objectV[i]->GetID() == ID) {
			W_SAFE_REMOVEREF(m_objectV[i]);
			m_objectV.erase(m_objectV.begin() + i);
			break;
		}
}

void WRigidBody::Enable() {
	m_physics = true; //mark rigid body as using physics

	if (m_rigidBody) //if the we already has a rigid body
	{
		//update rotation & position
		OnStateChange(CHANGE_MOTION);
		OnStateChange(CHANGE_ROTATION);
	}
}

void WRigidBody::Disable() {
	m_physics = false; //mark rigid body as not using physics
}

bool WRigidBody::Enabled() const {
	return m_physics;
}

WError WRigidBody::BuildFromGeometries(W_RIGIDBODYTYPE type, WGeometry* const* geometries, UINT numGeometries,
									   bool bFixed, bool bSimplify, void* mopp, UINT moppsize) {
	if (!((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return W_PHYSICSNOTINITIALIZED;

	if (!geometries || !numGeometries)
		return W_INVALIDPARAM;
	if (!geometries[0]->Valid())
		return W_INVALIDPARAM;
	if (!geometries[0]->GetNumVertices())
		return W_INVALIDPARAM;
	if (type != RIGIDBODY_SPHERE && type != RIGIDBODY_BOX && type != RIGIDBODY_COMPLEX)
		return W_INVALIDPARAM;

	//destroy the currently in use body if it exists
	Destroy();

	hkpRigidBodyCinfo rigidBodyInfo;
	hkReal mass = 10.0f;

	m_rbType = type;
	m_simpleMesh = bSimplify;
	if (!(type == RIGIDBODY_COMPLEX && !bSimplify)) //otherwise its a mopp, this information isnt needed
	{
		//count the vertices
		m_rbNumVerts = 0;
		for (UINT i = 0; i < numGeometries; i++)
			m_rbNumVerts += geometries[i]->GetNumVertices();

		m_rbVerts = new WVector3[m_rbNumVerts];

		m_rbNumVerts = 0; //set to 0 to apply offsets (will be increased to normal while copying vertices so no worries)
		for (UINT i = 0; i < numGeometries; i++) {
			W_VERTEX_DESCRIPTION desc = geometries[i]->GetVertexDescription();
			UINT structSize = desc.GetSize();
			UINT posoff = desc.GetOffset("position");
			if (posoff != -1) {
				void* verts;
				UINT geometryVerts = geometries[i]->GetNumVertices();
				geometries[i]->MapVertexBuffer(&verts) , true);
				for (UINT n = 0; n < geometryVerts; n++) {
					WVector3 curVert;
					//offset by (m_rbNumVerts*structSize) to move to current location
					memcpy(&curVert, &((char*)verts)[n*structSize + posoff], sizeof WVector3);
					m_rbVerts[m_rbNumVerts + n] = curVert;
				}
				geometries[i]->UnmapVertexBuffer();

				m_rbNumVerts += geometries[i]->GetNumVertices();
			}
		}
	}
	//find out the farthest vertex from origin so it will be used for the shape's size (box/sphere)
	float minx = 0, miny = 0, minz = 0;
	float maxx = 0, maxy = 0, maxz = 0;
	WVector3 farthestVert(0, 0, 0);

	for (UINT i = 0; i < m_rbNumVerts; i++) {
		if (m_rbVerts[i].x > maxx)
			maxx = m_rbVerts[i].x;
		if (m_rbVerts[i].x < minx)
			minx = m_rbVerts[i].x;

		if (m_rbVerts[i].y > maxy)
			maxy = m_rbVerts[i].y;
		if (m_rbVerts[i].y < miny)
			miny = m_rbVerts[i].y;

		if (m_rbVerts[i].z > maxz)
			maxz = m_rbVerts[i].z;
		if (m_rbVerts[i].z < minz)
			minz = m_rbVerts[i].z;

		//no sqrt for two reasons: we don't need to know the actual distance, just comparing, and sqrt
		//can sometimes be slow
		if (abs(m_rbVerts[i].x*m_rbVerts[i].x + m_rbVerts[i].y*m_rbVerts[i].y +
				m_rbVerts[i].z*m_rbVerts[i].z) >
			abs(farthestVert.x*farthestVert.x + farthestVert.y*farthestVert.y + farthestVert.z*farthestVert.z))
			farthestVert = m_rbVerts[i];
	}

	//create a box body
	if (type == RIGIDBODY_BOX) {
		hkVector4 boxSize(max(maxx, minx), max(maxy, miny), max(maxz, minz));

		//no dimension should be zero
		if (abs(boxSize(0)) <= 0.001f) //too small edge, enlargen it
			boxSize(0) += 0.001f;
		if (abs(boxSize(1)) <= 0.001f) //too small edge, enlargen it
			boxSize(1) += 0.001f;
		if (abs(boxSize(2)) <= 0.001f) //too small edge, enlargen it
			boxSize(2) += 0.001f;

		//initiate the body
		hkpMassProperties massProperties;
		hkpInertiaTensorComputer::computeBoxVolumeMassProperties(boxSize, mass, massProperties);

		rigidBodyInfo.m_mass = massProperties.m_mass;
		rigidBodyInfo.m_centerOfMass = massProperties.m_centerOfMass;
		rigidBodyInfo.m_inertiaTensor = massProperties.m_inertiaTensor;
		rigidBodyInfo.m_shape = new hkpBoxShape(boxSize);
	} else if (type == RIGIDBODY_SPHERE) {
		//get radius (sphere size)
		float radius = sqrt(float(abs(farthestVert.x*farthestVert.x +
									  farthestVert.y*farthestVert.y +
									  farthestVert.z*farthestVert.z)));
		if (abs(radius) <= 0.001f) //too small
			radius += 0.001f;

		//initiate the shape
		hkpMassProperties massProperties;
		hkpInertiaTensorComputer::computeSphereVolumeMassProperties(radius, mass, massProperties);

		rigidBodyInfo.m_mass = massProperties.m_mass;
		rigidBodyInfo.m_centerOfMass = massProperties.m_centerOfMass;
		rigidBodyInfo.m_inertiaTensor = massProperties.m_inertiaTensor;
		rigidBodyInfo.m_shape = new hkpSphereShape(radius);
	} else if (type == RIGIDBODY_COMPLEX) {
		if (m_simpleMesh) {
			//convert the vertex buffer to an array of havok vectors
			float* vertices = new float[m_rbNumVerts * 4];

			for (UINT i = 0; i < m_rbNumVerts; i++) {
				vertices[i * 4 + 0] = m_rbVerts[i].x;
				vertices[i * 4 + 1] = m_rbVerts[i].y;
				vertices[i * 4 + 2] = m_rbVerts[i].z;
				vertices[i * 4 + 3] = 0.0f;
			}

			hkStridedVertices stridedVerts;
			stridedVerts.m_numVertices = m_rbNumVerts;
			stridedVerts.m_striding = sizeof hkVector4;
			stridedVerts.m_vertices = &(vertices[0]);

			hkArray<hkVector4> dummyPlaneEquations;

			//initiate the shape
			rigidBodyInfo.m_shape = new hkpConvexVerticesShape(stridedVerts, dummyPlaneEquations);
			hkpInertiaTensorComputer::setShapeVolumeMassProperties(rigidBodyInfo.m_shape, mass, rigidBodyInfo);

			W_SAFE_DELETE_ARRAY(vertices);
		} else {
			hkpExtendedMeshShape* geometry = new hkpExtendedMeshShape();
			geometry->setRadius(0.05f);

			//vector<void*> memToFree;
			for (UINT i = 0; i < numGeometries; i++) {
				UINT posoff = geometries[i]->GetVertexDescription().GetOffset("position");
				if (posoff != -1) {
					hkpExtendedMeshShape::TrianglesSubpart part;

					part.m_vertexStriding = geometries[i]->GetVertexDescription().GetSize();//sizeof WVector3;
					part.m_numVertices = geometries[i]->GetNumVertices();
					//WVector3* tmpVertBase = new WVector3[part.m_numVertices];
					//part.m_vertexBase = (float*)tmpVertBase;
					//UINT structSize = geometries[i]->GetStructureSize ( );
					//void* verts;
					geometries[i]->MapVertexBuffer((void**)&part.m_vertexBase, true);
					part.m_vertexBase += posoff; // so that we are pointing at positions, stride remains the same
						//for ( UINT n = 0; n < part.m_numVertices; n++ )
						//	memcpy ( &tmpVertBase[n], &((char*)verts)[n*structSize], sizeof WVector3 );

						DWORD* indices = nullptr;
					UINT geometryIndices = geometries[i]->GetNumIndices();
					geometries[i]->MapIndexBuffer(&indices), true);
					//if (geometries[i]->GetTopology() == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
					part.m_indexBase = indices;
					part.m_indexStriding = 3 * sizeof DWORD; //request a triangle list
					part.m_numTriangleShapes = geometryIndices / 3;
					part.m_stridingType = hkpExtendedMeshShape::INDICES_INT32;
					/*} else if (geometries[i]->GetTopology() == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP) {
						part.m_indexBase = indices;
						part.m_indexStriding = 2 * sizeof DWORD; //request a triangle strip
						part.m_numTriangleShapes = (geometryIndices - 1) / 2 + 1; //1 to begin with, then a triangle for every 2
						part.m_stridingType = hkpExtendedMeshShape::INDICES_INT32;
					}*/

					geometry->addTrianglesSubpart(part);

					geometries[i]->UnmapVertexBuffer();
					geometries[i]->UnmapIndexBuffer();

					//memToFree.push_back ( indices );
					//memToFree.push_back ( tmpVertBase );
				}
			}

			hkpMoppCode* code = nullptr;
			if (mopp) {
				hkIArchive ia(mopp, moppsize);
				if (ia.isOk())
					code = hkpMoppCodeStreamer::readMoppCodeFromArchive(ia);
			} else {
				hkpMoppCompilerInput mci;
				code = hkpMoppUtility::buildCode(geometry, mci);
			}
			if (((WHavokPhysics*)m_app->PhysicsComponent)->RigidBodyManager->IsMoppSavingEnabled()) {
				W_SAFE_DELETE_ARRAY(m_rbVerts);
				m_rbNumVerts = 0;
				m_moppcodeSize = code->getCodeSize() + code->getAllocatedSize();
				m_moppcode = W_SAFE_ALLOC(m_moppcodeSize);
				m_numGeometries = numGeometries;
				m_geometryIDs = new UINT[numGeometries];
				for (UINT i = 0; i < numGeometries; i++)
					m_geometryIDs[i] = geometries[i]->GetID();
				hkOArchive oa(m_moppcode, m_moppcodeSize);
				hkpMoppCodeStreamer::writeMoppCodeToArchive(code, oa);
			}
			rigidBodyInfo.m_shape = new hkpMoppBvTreeShape(geometry, code);

			//for ( UINT i = 0; i < memToFree.size ( ); i++ )
			//	delete[] memToFree[i];

			geometry->removeReference();
			code->removeReference();
		}
	}

	//build the rigid body
	rigidBodyInfo.m_position = hkVector4(GetPositionX(), GetPositionY(), GetPositionZ());
	rigidBodyInfo.m_rotation = WRigidBody::GetRotationQuat();
	rigidBodyInfo.m_motionType = bFixed ? hkpMotion::MOTION_FIXED : hkpMotion::MOTION_DYNAMIC;
	rigidBodyInfo.m_qualityType = bFixed ? HK_COLLIDABLE_QUALITY_FIXED : HK_COLLIDABLE_QUALITY_MOVING;
	m_rigidBody = new hkpRigidBody(rigidBodyInfo);
	rigidBodyInfo.m_shape->removeReference();

	//add the rigid body to the world
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->addEntity(m_rigidBody);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();

	return WError(W_SUCCEEDED);
}

void WRigidBody::Destroy() {
	//delete the physics informations if it exists
	if (m_rigidBody) {
		((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
		m_rigidBody->activate(); //update simulation islands
		((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->removeEntity(m_rigidBody);
		((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
		m_rigidBody->removeReference();
		m_rigidBody = nullptr;
		W_SAFE_DELETE_ARRAY(m_rbVerts);
		W_SAFE_FREE(m_moppcode);
		W_SAFE_DELETE(m_geometryIDs);
		m_moppcodeSize = 0;
		m_numGeometries = 0;
		m_rbNumVerts = 0;
	}
}

void WRigidBody::SetLinearVelocity(float velX, float velY, float velZ) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setLinearVelocity(hkVector4(velX, velY, velZ));
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetAngularVelocity(float velX, float velY, float velZ) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setAngularVelocity(hkVector4(velX, velY, velZ));
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetLinearDamping(float power) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setLinearDamping(power);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetAngularDamping(float power) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setAngularDamping(power);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetMaxLinearVelocity(float fScalarVelocity) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setMaxLinearVelocity(fScalarVelocity);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetMaxAngularVelocity(float fScalarVelocity) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setMaxAngularVelocity(fScalarVelocity);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetMotionType(W_MOTIONTYPE type) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setMotionType(type == MOTIONTYPE_DYNAMIC ? hkpMotion::MOTION_DYNAMIC : type == MOTIONTYPE_STATIC ?
							   hkpMotion::MOTION_FIXED : hkpMotion::MOTION_KEYFRAMED);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetBouncingPower(float bouncing) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;
	if (bouncing > 1)
		bouncing = 1;
	if (bouncing < 0)
		bouncing = 0;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setRestitution(bouncing);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetMass(float mass) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setMass(mass);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetMassCenter(float x, float y, float z) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setCenterOfMassLocal(hkVector4(x, y, z));
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetFriction(float friction) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setFriction(friction);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

void WRigidBody::SetCollisionQuality(W_COLLISIONQUALITY quality) {
	//Don't bother if we don't have a rigid body
	if (!m_rigidBody || !((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld())
		return;

	hkpCollidableQualityType cqt;
	switch (quality) {
	case COLLISIONQUALITY_FIXED:
		cqt = HK_COLLIDABLE_QUALITY_FIXED;
		break;
	case COLLISIONQUALITY_KEYFRAMED:
		cqt = HK_COLLIDABLE_QUALITY_KEYFRAMED;
		break;
	case COLLISIONQUALITY_DEBRIS:
		cqt = HK_COLLIDABLE_QUALITY_DEBRIS;
		break;
	case COLLISIONQUALITY_DEBRIS_SIMPLE_TOI:
		cqt = HK_COLLIDABLE_QUALITY_DEBRIS_SIMPLE_TOI;
		break;
	case COLLISIONQUALITY_MOVING:
		cqt = HK_COLLIDABLE_QUALITY_MOVING;
		break;
	case COLLISIONQUALITY_CRITICAL:
		cqt = HK_COLLIDABLE_QUALITY_CRITICAL;
		break;
	case COLLISIONQUALITY_BULLET:
		cqt = HK_COLLIDABLE_QUALITY_BULLET;
		break;
	case COLLISIONQUALITY_USER:
		cqt = HK_COLLIDABLE_QUALITY_USER;
		break;
	case COLLISIONQUALITY_CHARACTER:
		cqt = HK_COLLIDABLE_QUALITY_CHARACTER;
		break;
	case COLLISIONQUALITY_KEYFRAMED_REPORTING:
		cqt = HK_COLLIDABLE_QUALITY_KEYFRAMED_REPORTING;
		break;
	default:
		cqt = HK_COLLIDABLE_QUALITY_MOVING;
	}

	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	m_rigidBody->setQualityType(cqt);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

hkpRigidBody* WRigidBody::GetRigidBody() const {
	return m_rigidBody;
}

W_RIGIDBODYTYPE WRigidBody::GetType() const {
	return m_rbType;
}

void WRigidBody::OnStateChange(STATE_CHANGE_TYPE type) {
	m_bAltered = true;
	if (!((WHavokPhysics*)m_app->PhysicsComponent)->Stepping()) //while stepping, info provided is taken off rigid body so no need to update it
	{
		//update the local info
		UpdateLocals();

		if (type == CHANGE_MOTION) {
			if (m_rigidBody) //update the rigid body position if it exists
			{
				((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();

				hkVector4 pos = hkVector4(GetPositionX(), GetPositionY(), GetPositionZ());
				hkVector4 oldPos = m_rigidBody->getPosition();
				m_rigidBody->setPosition(pos);

				((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
			}
		} else if (type == CHANGE_ROTATION) {
			if (m_rigidBody)//set the rigid body rotation if it exists
			{
				((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();

				hkQuaternion q = WRigidBody::GetRotationQuat();
				m_rigidBody->setRotation(q);

				((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
			}
		}
	}

	//update bound objects to be at the same position/rotation as the rigid body
	for (UINT i = 0; i < m_objectV.size(); i++) {
		if (!m_objectV[i]) //object is invalid, erase it
		{
			m_objectV.erase(m_objectV.begin() + i);
			i--;
		} else {
			m_objectV[i]->SetPosition(GetPosition());
			m_objectV[i]->SetToRotation(this);
		}
	}
}

WMatrix WRigidBody::GetWorldMatrix() {
	UpdateLocals(); //return matrix without offsets
	return m_WorldM;
}

void WRigidBody::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		WVector3 _up = GetUVector();
		WVector3 _look = GetLVector();
		WVector3 _right = GetRVector();
		WVector3 _pos = GetPosition();

		//
		//the world matrix is the view matrix's inverse
		//so we build a normal view matrix and inverse it
		//

		//build world matrix
		float x = -WVec3Dot(_right, _pos);
		float y = -WVec3Dot(_up, _pos);
		float z = -WVec3Dot(_look, _pos);
		(m_WorldM)(0, 0) = _right.x; (m_WorldM)(0, 1) = _up.x; (m_WorldM)(0, 2) = _look.x; (m_WorldM)(0, 3) = 0.0f;
		(m_WorldM)(1, 0) = _right.y; (m_WorldM)(1, 1) = _up.y; (m_WorldM)(1, 2) = _look.y; (m_WorldM)(1, 3) = 0.0f;
		(m_WorldM)(2, 0) = _right.z; (m_WorldM)(2, 1) = _up.z; (m_WorldM)(2, 2) = _look.z; (m_WorldM)(2, 3) = 0.0f;
		(m_WorldM)(3, 0) = x;        (m_WorldM)(3, 1) = y;     (m_WorldM)(3, 2) = z;       (m_WorldM)(3, 3) = 1.0f;
		m_WorldM = WMatrixInverse(m_WorldM);
	}
}

hkQuaternion WRigidBody::GetRotationQuat() {
	//build a quaternion and return it
	WMatrix matrix = GetWorldMatrix();
	hkReal w = sqrt(1 + matrix(0, 0) + matrix(1, 1) + matrix(2, 2)) / 2;
	hkReal x = (matrix(2, 1) - matrix(1, 2)) / (4 * w);
	hkReal y = (matrix(0, 2) - matrix(2, 0)) / (4 * w);
	hkReal z = (matrix(1, 0) - matrix(0, 1)) / (4 * w);

	hkQuaternion q(x, y, z, w);

	return q;
}

bool WRigidBody::Valid() const {
	return (m_rigidBody != nullptr);
}

WConstraintManager::WConstraintManager(Wasabi* app) : WManager<WConstraint>(m_app) {
}

std::string WConstraintManager::GetTypeName() const {
	return "Constraint";
}

//constraint class implemention
WConstraint::WConstraint(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_world = ((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld();
	m_rb1 = m_rb2 = nullptr;
	m_constraint = nullptr;
	m_breakableConstraint = nullptr;
	m_instance = nullptr;

	//register the Constraint
	((WHavokPhysics*)m_app->PhysicsComponent)->ConstraintManager->AddEntity(this);
}

WConstraint::~WConstraint() {
	if (m_instance) {
		((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
		((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->removeConstraint(m_instance);
		((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
	}
	m_instance->removeReference();
	if (m_breakableConstraint)
		m_breakableConstraint->removeReference();

	//unregister the Constraint
	((WHavokPhysics*)m_app->PhysicsComponent)->ConstraintManager->RemoveEntity(this);
}

std::string WConstraint::GetTypeName() const {
	return "Constraint";
}

void WConstraint::SetBreakingThreshold(float threshold) {
	if (m_breakableConstraint)
		m_breakableConstraint->setThreshold(threshold);
}

void WConstraint::SetMaxLinearImpulse(float impulse) {
	if (m_constraint)
		m_constraint->setMaxLinearImpulse(impulse);
	else if (m_breakableConstraint)
		m_breakableConstraint->setMaxLinearImpulse(impulse);
}

hkpConstraintData* WConstraint::GetConstraint() const {
	if (m_constraint)
		return m_constraint;
	else if (m_breakableConstraint)
		return m_breakableConstraint;

	return nullptr;
}

bool WConstraint::Valid() const {
	if (!m_constraint && !m_breakableConstraint)
		return false;

	return (m_constraint->isValid() && m_rb1 && m_rb2);
}

/*****************************************************************
H I N G E   C O N S T R A I N T
*****************************************************************/
WHinge::WHinge(Wasabi* const app, unsigned int ID) : WConstraint(m_app, ID) {
	m_hc = nullptr;
}
WHinge::~WHinge() {
	if (m_hc)
		m_hc->removeReference();
}
void WHinge::Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable) {
	if (!m_world || !rb1->GetRigidBody() || !rb2->GetRigidBody())
		return;

	m_rb1 = rb1->GetRigidBody();
	m_rb2 = rb2->GetRigidBody();

	//connection point (midpoint of the bodies positions)
	hkVector4 pivot((rb1->GetPositionX() + rb2->GetPositionX()) / 2,
		(rb1->GetPositionY() + rb2->GetPositionY()) / 2,
					(rb1->GetPositionZ() + rb2->GetPositionZ()) / 2);
	hkVector4 axis(0.0f, 0.0f, 1.0f); //align to Z

									  //create hinge constraint
	m_hc = new hkpHingeConstraintData();
	m_hc->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(), pivot, axis);

	if (breakable) {
		m_breakableConstraint = new hkpBreakableConstraintData(m_hc);
		m_breakableConstraint->setRemoveWhenBroken(false);
		m_instance = new hkpConstraintInstance(
			m_rb1, m_rb2, m_breakableConstraint);
	} else
		m_instance = new hkpConstraintInstance(m_rb1, m_rb2, m_hc);

	m_world->lock();
	m_world->addConstraint(m_instance);
	m_world->unlock();

	m_constraint = m_hc;
}

void WHinge::SetPositionWorld(float x, float y, float z, float axisX, float axisY, float axisZ) {
	if (m_hc)
		m_hc->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(x, y, z),
							  hkVector4(axisX, axisY, axisZ));
}

void WHinge::SetPositionWorld(WVector3 position, WVector3 axis) {
	if (m_hc)
		m_hc->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(position.x, position.y, position.z),
							  hkVector4(axis.x, axis.y, axis.z));
}

void WHinge::SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2,
							  float axisX1, float axisY1, float axisZ1, float axisX2, float axisY2, float axisZ2) {
	if (m_hc)
		m_hc->setInBodySpace(hkVector4(x1, y1, z1), hkVector4(x2, y2, z2),
							 hkVector4(axisX1, axisY1, axisZ1),
							 hkVector4(axisX2, axisY2, axisZ2));
}

void WHinge::SetPositionLocal(WVector3 position1, WVector3 position2, WVector3 axis1, WVector3 axis2) {
	if (m_hc)
		m_hc->setInBodySpace(hkVector4(position1.x, position1.y, position1.z),
							 hkVector4(position2.x, position2.y, position2.z),
							 hkVector4(axis1.x, axis1.y, axis1.z),
							 hkVector4(axis2.x, axis2.y, axis2.z));
}

/*****************************************************************
L I M I T E D   H I N G E   C O N S T R A I N T
*****************************************************************/
WLimitedHinge::WLimitedHinge(Wasabi* const app, unsigned int ID) : WConstraint(m_app, ID) {
	m_lhc = nullptr;
}

WLimitedHinge::~WLimitedHinge() {
	if (m_lhc)
		m_lhc->removeReference();
}

void WLimitedHinge::Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable) {
	if (!m_world || !rb1->GetRigidBody() || !rb2->GetRigidBody())
		return;

	m_rb1 = rb1->GetRigidBody();
	m_rb2 = rb2->GetRigidBody();

	//connection point (midpoint of the bodies positions)
	hkVector4 pivot((rb1->GetPositionX() + rb2->GetPositionX()) / 2,
		(rb1->GetPositionY() + rb2->GetPositionY()) / 2,
					(rb1->GetPositionZ() + rb2->GetPositionZ()) / 2);
	hkVector4 axis(0.0f, 0.0f, 1.0f); //align to Z

									  //create hinge constraint
	m_lhc = new hkpLimitedHingeConstraintData();
	m_lhc->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(), pivot, axis);

	if (breakable) {
		m_breakableConstraint = new hkpBreakableConstraintData(m_lhc);
		m_breakableConstraint->setRemoveWhenBroken(false);
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_breakableConstraint);
	} else
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_lhc);

	m_world->lock();
	m_world->addConstraint(m_instance);
	m_world->unlock();

	m_constraint = m_lhc;
}

void WLimitedHinge::SetPositionWorld(float x, float y, float z, float axisX, float axisY, float axisZ) {
	if (m_lhc)
		m_lhc->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							   hkVector4(x, y, z),
							   hkVector4(axisX, axisY, axisZ));
}

void WLimitedHinge::SetPositionWorld(WVector3 position, WVector3 axis) {
	if (m_lhc)
		m_lhc->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							   hkVector4(position.x, position.y, position.z),
							   hkVector4(axis.x, axis.y, axis.z));
}

void WLimitedHinge::SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2,
									 float axisX1, float axisY1, float axisZ1, float axisX2, float axisY2, float axisZ2,
									 float axisX1perp, float axisY1perp, float axisZ1perp, float axisX2perp, float axisY2perp, float axisZ2perp) {
	if (m_lhc)
		m_lhc->setInBodySpace(hkVector4(x1, y1, z1), hkVector4(x2, y2, z2),
							  hkVector4(axisX1, axisY1, axisZ1),
							  hkVector4(axisX2, axisY2, axisZ2),
							  hkVector4(axisX2perp, axisY2perp, axisZ2perp),
							  hkVector4(axisX2perp, axisY2perp, axisZ2perp));
}

void WLimitedHinge::SetPositionLocal(WVector3 position1, WVector3 position2,
									 WVector3 axis1, WVector3 axis2, WVector3 axis1perp, WVector3 axis2perp) {
	if (m_lhc)
		m_lhc->setInBodySpace(hkVector4(position1.x, position1.y, position1.z),
							  hkVector4(position2.x, position2.y, position2.z),
							  hkVector4(axis1.x, axis1.y, axis1.z),
							  hkVector4(axis2.x, axis2.y, axis2.z),
							  hkVector4(axis2perp.x, axis2perp.y, axis2perp.z),
							  hkVector4(axis2perp.x, axis2perp.y, axis2perp.z));
}

void WLimitedHinge::SetMaximumAngle(float angle) {
	if (m_lhc)
		m_lhc->setMaxAngularLimit(W_DEGTORAD(angle));
}

void WLimitedHinge::SetMinimumAngle(float angle) {
	if (m_lhc)
		m_lhc->setMinAngularLimit(W_DEGTORAD(angle));
}

void WLimitedHinge::SetTauFactor(float factor) {
	if (m_lhc)
		m_lhc->setAngularLimitsTauFactor(factor);
}

void WLimitedHinge::SetMaxFrictionTorque(float torque) {
	if (m_lhc)
		m_lhc->setMaxFrictionTorque(torque);
}

void WLimitedHinge::SetMotor(void(__cdecl *cb)(const class hkpCallbackConstraintMotor& motor,
											   const class hkpConstraintMotorInput* input, class hkpConstraintMotorOutput* output),
							float fMinForce, float fMaxForce) {
	if (m_lhc) {
		hkpCallbackConstraintMotor* ccm = new hkpCallbackConstraintMotor(hkpCallbackConstraintMotor::CALLBACK_MOTOR_TYPE_USER_0, cb);
		m_lhc->setMotor(ccm);
		ccm->setMinMaxForces(fMinForce, fMaxForce);
	}
}

void WLimitedHinge::EnableMotor() {
	if (m_lhc)
		m_lhc->setMotorActive(m_instance, true);
}

void WLimitedHinge::DisableMotor() {
	if (m_lhc)
		m_lhc->setMotorActive(m_instance, false);
}

void WLimitedHinge::SetMotorTargetAngle(float ang) {
	if (m_lhc)
		m_lhc->setMotorTargetAngle(W_DEGTORAD(ang));
}


/*****************************************************************
B A L L   A N D   S O C K E T   C O N S T R A I N T
*****************************************************************/
WBallAndSocket::WBallAndSocket(Wasabi* const app, unsigned int ID) : WConstraint(m_app, ID) {
	m_bs = nullptr;
}

WBallAndSocket::~WBallAndSocket() {
	if (m_bs)
		m_bs->removeReference();
}

void WBallAndSocket::Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable) {
	if (!m_world || !rb1->GetRigidBody() || !rb2->GetRigidBody())
		return;

	m_rb1 = rb1->GetRigidBody();
	m_rb2 = rb2->GetRigidBody();

	//connection point (midpoint of the bodies positions)
	hkVector4 pivot((rb1->GetPositionX() + rb2->GetPositionX()) / 2,
		(rb1->GetPositionY() + rb2->GetPositionY()) / 2,
					(rb1->GetPositionZ() + rb2->GetPositionZ()) / 2);

	//create ball & socket constraint
	m_bs = new hkpBallAndSocketConstraintData();
	m_bs->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(), pivot);

	if (breakable) {
		m_breakableConstraint = new hkpBreakableConstraintData(m_bs);
		m_breakableConstraint->setRemoveWhenBroken(false);
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_breakableConstraint);
	} else
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_bs);

	m_world->lock();
	m_world->addConstraint(m_instance);
	m_world->unlock();

	m_constraint = m_bs;
}

void WBallAndSocket::SetPositionWorld(float x, float y, float z) {
	if (m_bs)
		m_bs->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(x, y, z));
}

void WBallAndSocket::SetPositionWorld(WVector3 position) {
	if (m_bs)
		m_bs->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(position.x, position.y, position.z));
}

void WBallAndSocket::SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2) {
	if (m_bs)
		m_bs->setInBodySpace(hkVector4(x1, y1, z1),
							 hkVector4(x2, y2, z2));
}

void WBallAndSocket::SetPositionLocal(WVector3 position1, WVector3 position2) {
	if (m_bs)
		m_bs->setInBodySpace(hkVector4(position1.x, position1.y, position1.z),
							 hkVector4(position2.x, position2.y, position2.z));
}

/*****************************************************************
S T I F F   S P R I N G   C O N S T R A I N T
*****************************************************************/
WStiffSpring::WStiffSpring(Wasabi* const app, unsigned int ID) : WConstraint(m_app, ID) {
	m_ss = nullptr;
}

WStiffSpring::~WStiffSpring() {
	if (m_ss)
		m_ss->removeReference();
}

void WStiffSpring::Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable) {
	if (!rb1 || !rb2)
		return;
	if (!m_world || !rb1->GetRigidBody() || !rb2->GetRigidBody())
		return;

	m_rb1 = rb1->GetRigidBody();
	m_rb2 = rb2->GetRigidBody();

	//positions
	hkVector4 pos1(rb1->GetPositionX(), rb1->GetPositionY(), rb1->GetPositionZ());
	hkVector4 pos2(rb2->GetPositionX(), rb2->GetPositionY(), rb2->GetPositionZ());

	//create stiff spring constraint
	m_ss = new hkpStiffSpringConstraintData();
	m_ss->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(), pos1, pos2);

	if (breakable) {
		m_breakableConstraint = new hkpBreakableConstraintData(m_ss);
		m_breakableConstraint->setRemoveWhenBroken(false);
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_breakableConstraint);
	} else
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_ss);

	m_world->lock();
	m_world->addConstraint(m_instance);
	m_world->unlock();

	m_constraint = m_ss;
}

void WStiffSpring::SetPositionWorld(float x, float y, float z, float x2, float y2, float z2) {
	if (m_ss)
		m_ss->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(x, y, z), hkVector4(x2, y2, z2));
}

void WStiffSpring::SetPositionWorld(WVector3 position1, WVector3 position2) {
	if (m_ss)
		m_ss->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(position1.x, position1.y, position1.z), hkVector4(position2.x, position2.y, position2.z));
}

void WStiffSpring::SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2, float length) {
	if (m_ss)
		m_ss->setInBodySpace(hkVector4(x1, y1, z1),
							 hkVector4(x2, y2, z2), length);
}

void WStiffSpring::SetPositionLocal(WVector3 position1, WVector3 position2, float length) {
	if (m_ss)
		m_ss->setInBodySpace(hkVector4(position1.x, position1.y, position1.z),
							 hkVector4(position2.x, position2.y, position2.z), length);
}


/*****************************************************************
P U L L E Y   C O N S T R A I N T
*****************************************************************/
WPulley::WPulley(Wasabi* const app, unsigned int ID) : WConstraint(m_app, ID) {
	m_pu = nullptr;
}

WPulley::~WPulley() {
	if (m_pu)
		m_pu->removeReference();
}

void WPulley::Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable) {
	if (!m_world || !rb1->GetRigidBody() || !rb2->GetRigidBody())
		return;

	m_rb1 = rb1->GetRigidBody();
	m_rb2 = rb2->GetRigidBody();

	//positions
	hkVector4 pos1(rb1->GetPositionX(), rb1->GetPositionY(), rb1->GetPositionZ());
	hkVector4 pos2(rb2->GetPositionX(), rb2->GetPositionY(), rb2->GetPositionZ());
	//world-space pulley parts stats 20 units above the objects
	hkVector4 pos3(rb1->GetPositionX(), rb1->GetPositionY() + 20, rb1->GetPositionZ());
	hkVector4 pos4(rb2->GetPositionX(), rb2->GetPositionY() + 20, rb2->GetPositionZ());

	//create ball & socket constraint
	m_pu = new hkpPulleyConstraintData();
	m_pu->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(), pos1, pos2, pos3, pos4, 3.0f);

	if (breakable) {
		m_breakableConstraint = new hkpBreakableConstraintData(m_pu);
		m_breakableConstraint->setRemoveWhenBroken(false);
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_breakableConstraint);
	} else
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_pu);

	m_world->lock();
	m_world->addConstraint(m_instance);
	m_world->unlock();

	m_constraint = m_pu;
}

void WPulley::SetPositionWorld(float x, float y, float z, float x2, float y2, float z2,
							   float px, float py, float pz, float px2, float py2, float pz2) {
	if (m_pu)
		m_pu->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(x, y, z), hkVector4(x2, y2, z2),
							  hkVector4(px, py, pz), hkVector4(px2, py2, pz2), m_pu->getLeverageOnBodyB());
}

void WPulley::SetPositionWorld(WVector3 position1, WVector3 position2, WVector3 pivot1, WVector3 pivot2) {
	if (m_pu)
		m_pu->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(position1.x, position1.y, position1.z),
							  hkVector4(position2.x, position2.y, position2.z),
							  hkVector4(pivot1.x, pivot1.y, pivot1.z),
							  hkVector4(pivot2.x, pivot2.y, pivot2.z), m_pu->getLeverageOnBodyB());
}

void WPulley::SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2,
							   float px, float py, float pz, float px2, float py2, float pz2) {
	if (m_pu)
		m_pu->setInBodySpace(m_rb1->getTransform(), m_rb2->getTransform(),
							 hkVector4(x1, y1, z1), hkVector4(x2, y2, z2),
							 hkVector4(px, py, pz), hkVector4(px2, py2, pz2), m_pu->getLeverageOnBodyB());
}

void WPulley::SetPositionLocal(WVector3 position1, WVector3 position2, WVector3 pivot1, WVector3 pivot2) {
	if (m_pu)
		m_pu->setInBodySpace(m_rb1->getTransform(), m_rb2->getTransform(),
							 hkVector4(position1.x, position1.y, position1.z),
							 hkVector4(position2.x, position2.y, position2.z),
							 hkVector4(pivot1.x, pivot1.y, pivot1.z),
							 hkVector4(pivot2.x, pivot2.y, pivot2.z), m_pu->getLeverageOnBodyB());
}

void WPulley::SetRopeLength(float length) {
	m_pu->setRopeLength(length);
}

void WPulley::SetLeverageOnB(float leverage) {
	m_pu->setLeverageOnBodyB(leverage);
}


/*****************************************************************
P R I S M A T I C   C O N S T R A I N T
*****************************************************************/
WPrismaticConstraint::WPrismaticConstraint(Wasabi* const app, unsigned int ID) : WConstraint(m_app, ID) {
	m_pr = nullptr;
}

WPrismaticConstraint::~WPrismaticConstraint() {
	if (m_pr)
		m_pr->removeReference();
}

void WPrismaticConstraint::Build(const WRigidBody* const rb1, const WRigidBody* const rb2, bool breakable) {
	if (!m_world || !rb1->GetRigidBody() || !rb2->GetRigidBody())
		return;

	m_rb1 = rb1->GetRigidBody();
	m_rb2 = rb2->GetRigidBody();

	//connection point (midpoint of the bodies positions)
	hkVector4 pivot((rb1->GetPositionX() + rb2->GetPositionX()) / 2,
		(rb1->GetPositionY() + rb2->GetPositionY()) / 2,
					(rb1->GetPositionZ() + rb2->GetPositionZ()) / 2);
	hkVector4 axis(0.0f, 0.0f, 1.0f); //align to Z

									  //create ball & socket constraint
	m_pr = new hkpPrismaticConstraintData();
	m_pr->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(), pivot, axis);

	if (breakable) {
		m_breakableConstraint = new hkpBreakableConstraintData(m_pr);
		m_breakableConstraint->setRemoveWhenBroken(false);
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_breakableConstraint);
	} else
		m_instance = new hkpConstraintInstance(m_rb1,
															m_rb2, m_pr);

	m_world->lock();
	m_world->addConstraint(m_instance);
	m_world->unlock();

	m_constraint = m_pr;
}

void WPrismaticConstraint::SetPositionWorld(float x, float y, float z, float axisX, float axisY, float axisZ) {
	if (m_pr)
		m_pr->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(x, y, z),
							  hkVector4(axisX, axisY, axisZ));
}

void WPrismaticConstraint::SetPositionWorld(WVector3 position, WVector3 axis) {
	if (m_pr)
		m_pr->setInWorldSpace(m_rb1->getTransform(), m_rb2->getTransform(),
							  hkVector4(position.x, position.y, position.z),
							  hkVector4(axis.x, axis.y, axis.z));
}

void WPrismaticConstraint::SetPositionLocal(float x1, float y1, float z1, float x2, float y2, float z2,
											float axisX1, float axisY1, float axisZ1, float axisX2, float axisY2, float axisZ2,
											float axisX1perp, float axisY1perp, float axisZ1perp, float axisX2perp, float axisY2perp, float axisZ2perp) {
	if (m_pr)
		m_pr->setInBodySpace(hkVector4(x1, y1, z1), hkVector4(x2, y2, z2),
							 hkVector4(axisX1, axisY1, axisZ1),
							 hkVector4(axisX2, axisY2, axisZ2),
							 hkVector4(axisX2perp, axisY2perp, axisZ2perp),
							 hkVector4(axisX2perp, axisY2perp, axisZ2perp));
}

void WPrismaticConstraint::SetPositionLocal(WVector3 position1, WVector3 position2,
											WVector3 axis1, WVector3 axis2, WVector3 axis1perp, WVector3 axis2perp) {
	if (m_pr)
		m_pr->setInBodySpace(hkVector4(position1.x, position1.y, position1.z),
							 hkVector4(position2.x, position2.y, position2.z),
							 hkVector4(axis1.x, axis1.y, axis1.z),
							 hkVector4(axis2.x, axis2.y, axis2.z),
							 hkVector4(axis2perp.x, axis2perp.y, axis2perp.z),
							 hkVector4(axis2perp.x, axis2perp.y, axis2perp.z));
}

void WPrismaticConstraint::SetMaximumPosition(float pos) {
	if (m_pr)
		m_pr->setMaxLinearLimit(pos);
}

void WPrismaticConstraint::SetMinimumPosition(float pos) {
	if (m_pr)
		m_pr->setMinLinearLimit(pos);
}

void WPrismaticConstraint::SetMaxFriction(float friction) {
	if (m_pr)
		m_pr->setMaxFrictionForce(friction);
}

void WPrismaticConstraint::SetMotor(void(__cdecl *cb)(const class hkpCallbackConstraintMotor& motor,
													  const class hkpConstraintMotorInput* input, class hkpConstraintMotorOutput* output), float fMinForce, float fMaxForce) {
	if (m_pr) {
		hkpCallbackConstraintMotor* ccm = new hkpCallbackConstraintMotor(hkpCallbackConstraintMotor::CALLBACK_MOTOR_TYPE_USER_0, cb);
		m_pr->setMotor(ccm);
		ccm->setMinMaxForces(fMinForce, fMaxForce);
	}
}

void WPrismaticConstraint::EnableMotor() {
	if (m_pr)
		m_pr->setMotorActive(m_instance, true);
}

void WPrismaticConstraint::DisableMotor() {
	if (m_pr)
		m_pr->setMotorActive(m_instance, false);
}

void WPrismaticConstraint::SetMotorTargetPos(float pos) {
	if (m_pr)
		m_pr->setMotorTargetPosition(pos);
}

WPhysicsActionManager::WPhysicsActionManager(Wasabi* app) : WManager<WPhysicsAction>(m_app) {
}

void WPhysicsActionManager::Update(float fDeltaTime) {
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	for (UINT j = 0; j < W_HASHTABLESIZE; j++)
		for (UINT i = 0; i < m_entities[j].size(); i++)
			m_entities[j][i]->Update(fDeltaTime);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
}

std::string WPhysicsActionManager::GetTypeName() const {
	return "PhysicsAction";
}

WPhysicsAction::WPhysicsAction(Wasabi* const app, WRigidBody* const body, unsigned int ID)
	: WBase(app, ID),
	hkpUnaryAction(body->GetRigidBody()) {
	m_rb = body;
	m_valid = false;

	//register the Constraint
	((WHavokPhysics*)m_app->PhysicsComponent)->ActionManager->AddEntity(this);
}

WPhysicsAction::~WPhysicsAction() {
	((WHavokPhysics*)m_app->PhysicsComponent)->ActionManager->RemoveEntity(this);
}

std::string WPhysicsAction::GetTypeName() const {
	return "PhysicsAction";
}

void WPhysicsAction::Initialize() {
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->lock();
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->addAction(this);
	((WHavokPhysics*)m_app->PhysicsComponent)->GetWorld()->unlock();
	m_valid = true;
}

void WPhysicsAction::Update(float fDeltaTime) {
	getEntity()->activate();
}

void WPhysicsAction::applyAction(const hkStepInfo& stepInfo) {
	//dummy
}

void WPhysicsAction::SetMotionDirection(WVector3 dir) {
}

WVector3 WPhysicsAction::GetMotionDirection() const {
	return WVector3(0, 0, 0);
}

bool WPhysicsAction::Valid() const {
	return m_valid;
}

WMarbleAction::WMarbleAction(Wasabi* const app, WRigidBody* const body, unsigned int ID)
	: WPhysicsAction(m_app, body, ID), m_forward(0, 0, 1) {
	m_impulseScale = 1.0f;
	m_currentAngle = 0.0f;
	m_curDir = WVector3(0, 0, 0);
	m_jumpMul = 12.0f;
	m_rotationIncrement = 0.1f;
	m_fRadius = 10.0f;
	m_bForward = false;
	m_bBackward = false;
	m_bLeft = false;
	m_bRight = false;
	m_bJump = false;
	m_bBrake = false;
	m_bStop = false;
	m_bOnGround = false;
}

WMarbleAction::~WMarbleAction() {
}

void WMarbleAction::applyAction(const hkStepInfo& stepInfo) {
	hkpRigidBody* rb = getRigidBody();

	m_bOnGround = ((WHavokPhysics*)m_app->PhysicsComponent)->RayCast(
		WPhysicsAction::m_rb->GetPosition(),
		WPhysicsAction::m_rb->GetPosition() - WVector3(0, m_fRadius + 2.0f, 0));

	// We need the delta time to record how long it's been since we last jumped
	// (to avoid jumping while in the air!)
	hkReal dt = stepInfo.m_deltaTime;

	// Get a "scale" to change the force of the impulse by,
	// depending on both the mass of the body, an an arbitrary "gain", eg 0.1
	hkReal scale = rb->getMass() * m_impulseScale;
	hkVector4 curVel = rb->getLinearVelocity();

	hkVector4 axis(0, 1, 0);
	hkQuaternion q(axis, m_currentAngle);
	hkVector4 f;
	f.setRotatedDir(q, m_forward);
	m_curDir = WVector3(f(0), f(1), f(2));

	if (m_bOnGround) {
		if (m_bForward) {
			hkVector4 imp;
			imp.setMul4(scale, f);
			rb->applyLinearImpulse(imp);
		}

		if (m_bBackward) {
			hkVector4 imp;
			imp.setMul4(-scale, f);
			rb->applyLinearImpulse(imp);
		}

		if (m_bForward) {
			curVel.sub4(f);
			curVel.mul4(-0.5f);
			rb->applyLinearImpulse(curVel);
			curVel.mul4(scale);
		}
		if (m_bLeft)
			m_currentAngle -= 3.141592653f * 2 * m_rotationIncrement * dt;
		if (m_bRight)
			m_currentAngle += 3.141592653f * 2 * m_rotationIncrement * dt;
		if (m_bForward) {
			curVel.mul4(-0.1);
			hkQuaternion q2(axis, m_currentAngle);
			f.setRotatedDir(q2, m_forward);
			hkVector4 imp2;
			imp2.setMul4(curVel.length4(), f);
			rb->applyLinearImpulse(imp2);
		}

		// If (brake) is pressed, oppose the motion direction to reduce speed
		if (m_bBrake) {
			curVel.mul4(-1.0f);
			rb->applyLinearImpulse(curVel);
		}

		// Jump
		if (m_bJump) {
			hkVector4 imp(0, rb->getMass() * m_jumpMul, 0);
			rb->applyLinearImpulse(imp);
		}
	} else {
		if (m_bLeft)
			m_currentAngle -= 3.141592653f * 2 * m_rotationIncrement * dt;
		if (m_bRight)
			m_currentAngle += 3.141592653f * 2 * m_rotationIncrement * dt;
	}

	// If (stop) is pressed, zero all velocities
	if (m_bStop) {
		hkVector4 zero;
		zero.setZero4();
		rb->setLinearVelocity(zero);
		rb->setAngularVelocity(zero);
	}
}

void WMarbleAction::SetForwardPressed(bool pressed) {
	m_bForward = pressed;
}

void WMarbleAction::SetBackwardPressed(bool pressed) {
	m_bBackward = pressed;
}

void WMarbleAction::SetTurnLeftPressed(bool pressed) {
	m_bLeft = pressed;
}

void WMarbleAction::SetTurnRightPressed(bool pressed) {
	m_bRight = pressed;
}

void WMarbleAction::SetJumpPressed(bool pressed) {
	m_bJump = pressed;
}

void WMarbleAction::SetBrakePressed(bool pressed) {
	m_bBrake = pressed;
}

void WMarbleAction::SetStopPressed(bool pressed) {
	m_bStop = pressed;
}

void WMarbleAction::SetMotionDirection(WVector3 dir) {
	WVector3 yVec(0.0f, 0.0f, 1.0f); //origin axis
	WVector3 _2dLook(dir.x, 0.0f, dir.z);
	_2dLook = WVec3Normalize(_2dLook);

	float dot = WVec3Dot(yVec, _2dLook); //get angle between the vectors
	float u = 1.0f;
	float v = sqrt(float(_2dLook.x*_2dLook.x + _2dLook.y*_2dLook.y + _2dLook.z*_2dLook.z));

	if (v == 0)
		return;

	float result = acos(float(dot / (u*v)));
	if (dir.x < 0.0f) //flip the angle if the object is in opposite direction
		result = -result;

	m_currentAngle = result;
}

void WMarbleAction::SetJumpingInfo(float fMultiplier, float fRadius) {
	m_jumpMul = fMultiplier;
	m_fRadius = fRadius;
}

void WMarbleAction::SetRotationInfo(float fMultiplier) {
	m_rotationIncrement = fMultiplier;
}

void WMarbleAction::SetAccelerationInfo(float fMultiplier) {
	m_impulseScale = fMultiplier;
}

bool WMarbleAction::IsOnGround() const {
	return m_bOnGround;
}

WVector3 WMarbleAction::GetMotionDirection() const {
	return m_curDir;
}

#endif