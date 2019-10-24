#include "Wasabi/Objects/WObject.hpp"
#include "Wasabi/Core/WCore.hpp"
#include "Wasabi/Core/WUtilities.hpp"
#include "Wasabi/Images/WImage.hpp"
#include "Wasabi/Images/WRenderTarget.hpp"
#include "Wasabi/Cameras/WCamera.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Materials/WEffect.hpp"
#include "Wasabi/Geometries/WGeometry.hpp"
#include "Wasabi/WindowAndInput/WWindowAndInputComponent.hpp"
#include "Wasabi/Animations/WAnimation.hpp"
#include "Wasabi/Animations/WSkeletalAnimation.hpp"

std::string WObjectManager::GetTypeName() const {
	return "Object";
}

WObjectManager::WObjectManager(class Wasabi* const app) : WManager<WObject>(app) {
}

WError WObjectManager::Load() {
	return WError(W_SUCCEEDED);
}

WObject* WObjectManager::CreateObject(uint32_t ID) const {
	return CreateObject(nullptr, 0, ID);
}

WObject* WObjectManager::CreateObject(WEffect* fx, uint32_t bindingSet, uint32_t ID) const {
	WObject* object = new WObject(m_app, fx, bindingSet, ID);
	return object;
}

WObject* WObjectManager::PickObject(double x, double y, bool bAnyHit, uint32_t iObjStartID, uint32_t iObjEndID, WVector3* _pt, WVector2* uv, uint32_t* faceIndex) const {
	struct pickStruct {
		WObject* obj;
		WVector3 pos;
		WVector2 uv;
		uint32_t face;
	};
	vector<pickStruct> pickedObjects;

	WCamera* cam = m_app->Renderer->GetRenderTarget(m_app->Renderer->GetPickingRenderStageName())->GetCamera();
	if (!cam)
		return nullptr;

	int Width = m_app->WindowAndInputComponent->GetWindowWidth();
	int Height = m_app->WindowAndInputComponent->GetWindowHeight();
	cam->Render(Width, Height);

	WMatrix P = cam->GetProjectionMatrix();
	WMatrix V = cam->GetViewMatrix();
	WMatrix inverseV = WMatrixInverse(V);

	// Compute picking ray in view space.
	float vx = (+2.0f*(float)x / Width  - 1.0f) / P(0, 0);
	float vy = (+2.0f*(float)y / Height - 1.0f) / P(1, 1);

	WVector3 pos(0.0f, 0.0f, 0.0f);
	WVector3 dir(vx, vy, 1.0f);

	pos = WVec3TransformCoord(pos, inverseV);
	dir = WVec3TransformNormal(dir, inverseV);

	for (uint32_t j = 0; j < W_HASHTABLESIZE; j++) {
		for (uint32_t i = (uint32_t)m_entities[j].size() - 1; i >= 0 && i != UINT_MAX; i--) {
			if (((m_entities[j][i]->GetID() >= iObjStartID && m_entities[j][i]->GetID() <= iObjEndID) ||
				(!iObjStartID && !iObjEndID)) && m_entities[j][i]->Valid()) {
				WObject* object = (WObject*)m_entities[j][i];
				if (object->Hidden())
					continue;

				WGeometry* temp = object->GetGeometry();
				if (temp) {
					//these calculations are per-subset
					WMatrix inverseW = WMatrixInverse(object->GetWorldMatrix());

					WVector3 subsetPos = WVec3TransformCoord(pos, inverseW);
					WVector3 subsetDir = WVec3TransformNormal(dir, inverseW);

					WVector3 boxPos = (temp->GetMaxPoint() + temp->GetMinPoint()) / 2.0f;
					WVector3 boxSize = (temp->GetMaxPoint() - temp->GetMinPoint()) / 2.0f;

					pickStruct p;
					p.obj = object;
					if (WUtil::RayIntersectBox(boxSize, subsetPos, subsetDir, boxPos)) {
						WVector3 pt;
						bool b = temp->Intersect(subsetPos, subsetDir, &pt, &p.uv, &p.face);
						WMatrix m = object->GetWorldMatrix();
						p.pos.x = (pt.x * m(0, 0)) + (pt.y * m(1, 0)) + (pt.z * m(2, 0)) + (1 * m(3, 0));
						p.pos.y = (pt.x * m(0, 1)) + (pt.y * m(1, 1)) + (pt.z * m(2, 1)) + (1 * m(3, 1));
						p.pos.z = (pt.x * m(0, 2)) + (pt.y * m(1, 2)) + (pt.z * m(2, 2)) + (1 * m(3, 2));

						if (b) {
							if (bAnyHit) {
								if (faceIndex) *faceIndex = p.face;
								if (_pt) *_pt = pt;
								if (uv) *uv = p.uv;
								return object;
							}
							pickedObjects.push_back(p);
						}
					}
				}
			}
		}
	}

	if (!pickedObjects.size())
		return 0;

	uint32_t nearest = 0;
	float distance = std::numeric_limits<float>::max();
	for (uint32_t i = 0; i < pickedObjects.size(); i++) {
		float fCurDist = WVec3Length(pickedObjects[i].pos - cam->GetPosition());
		if (fCurDist < distance) {
			nearest = i;
			distance = fCurDist;
		}
	}

	if (_pt) *_pt = pickedObjects[nearest].pos;
	if (uv) *uv = pickedObjects[nearest].uv;
	if (faceIndex) *faceIndex = pickedObjects[nearest].face;
	return pickedObjects[nearest].obj;
}

WInstance::WInstance() {
	m_scale = WVector3(1.0f, 1.0f, 1.0f);
}

WInstance::~WInstance() {
}

void WInstance::Scale(float x, float y, float z) {
	m_scale = WVector3(x, y, z);
	m_bAltered = true;
}

void WInstance::Scale(WVector3 scale) {
	m_scale = scale;
	m_bAltered = true;
}

WVector3 WInstance::GetScale() const {
	return m_scale;
}

WMatrix WInstance::GetWorldMatrix() {
	UpdateLocals();
	WMatrix m;
	m = m_worldM;
	m(0, 3) = 0;
	m(1, 3) = 0;
	m(2, 3) = 0;
	return m;
}

bool WInstance::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		m_worldM = ComputeTransformation();

		//scale matrix
		m_worldM = WScalingMatrix(m_scale) * m_worldM;

		if (IsBound())
			m_worldM *= GetBindingMatrix();

		m_worldM(0, 3) = m_worldM(3, 0);
		m_worldM(1, 3) = m_worldM(3, 1);
		m_worldM(2, 3) = m_worldM(3, 2);

		return true;
	}

	return false;
}

void WInstance::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type);
	m_bAltered = true;
}

WObject::WObject(Wasabi* const app, uint32_t ID) : WObject(app, nullptr, 0, ID) {}
WObject::WObject(Wasabi* const app, WEffect* fx, uint32_t bindingSet, uint32_t ID) : WFileAsset(app, ID), m_instanceV(0) {
	m_geometry = nullptr;
	m_animation = nullptr;

	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;

	m_WorldM = WMatrix();
	m_scale = WVector3(1.0f, 1.0f, 1.0f);

	m_instanceTexture = nullptr;

	if (fx)
		AddEffect(fx, bindingSet);

	app->ObjectManager->AddEntity(this);
}

WObject::~WObject() {
	W_SAFE_REMOVEREF(m_geometry);
	W_SAFE_REMOVEREF(m_animation);

	DestroyInstancingResources();

	m_app->ObjectManager->RemoveEntity(this);
}

std::string WObject::_GetTypeName() {
	return "Object";
}

std::string WObject::GetTypeName() const {
	return _GetTypeName();
}

void WObject::SetID(uint32_t newID) {
	m_app->ObjectManager->RemoveEntity(this);
	m_ID = newID;
	m_app->ObjectManager->AddEntity(this);
}

void WObject::SetName(std::string newName) {
	m_name = newName;
	m_app->ObjectManager->OnEntityNameChanged(this, newName);
}

bool WObject::Valid() const {
	return m_geometry && m_geometry->Valid();
}

void WObject::OnMaterialAdded(WMaterial* newMaterial) {
	newMaterial->SetVariable<WColor>("color", WColor(0.0f, 0.0f, 0.0f, 0.0f));
	newMaterial->SetVariable<float>("specularPower", 1.0f);
	newMaterial->SetVariable<float>("specularIntensity", 0.0f);
	newMaterial->SetVariable<int>("isTextured", 1);
}

bool WObject::WillRender(WRenderTarget* rt) {
	if (Valid() && !m_hidden) {
		WCamera* cam = rt->GetCamera();
		if (m_bFrustumCull) {
			if (!InCameraView(cam))
				return false;
		}
		return true;
	}
	return false;
}

void WObject::Render(WRenderTarget* rt, WMaterial* material, bool updateInstances) {
	if (updateInstances)
		_UpdateInstanceBuffer();

	bool is_animated = m_animation && m_animation->Valid() && m_geometry->IsRigged();
	bool is_instanced = m_instanceV.size() > 0;

	if (material) {
		WMatrix worldM = GetWorldMatrix();
		material->SetVariable<WMatrix>("worldMatrix", worldM);
		// animation variables
		material->SetVariable<int>("isAnimated", is_animated ? 1 : 0);
		material->SetVariable<int>("isInstanced", is_instanced ? 1 : 0);
		if (is_animated) {
			WImage* animTex = m_animation->GetTexture();
			material->SetTexture("animationTexture", animTex);
		}
		// instancing variables
		if (is_instanced) {
			material->SetTexture("instancingTexture", m_instanceTexture);
		}
		material->Bind(rt);
	}

	WError err = m_geometry->Draw(rt, std::numeric_limits<uint32_t>::max(), std::max((uint32_t)m_instanceV.size(), (uint32_t)1), is_animated);
	(void)err;
}

WError WObject::SetGeometry(class WGeometry* geometry) {
	if (m_geometry)
		m_geometry->RemoveReference();

	m_geometry = geometry;
	if (geometry) {
		m_geometry->AddReference();
	}

	return WError(W_SUCCEEDED);
}

WError WObject::SetAnimation(class WAnimation* animation) {
	if (m_animation)
		m_animation->RemoveReference();

	m_animation = animation;
	if (animation) {
		m_animation->AddReference();
	}

	return WError(W_SUCCEEDED);
}

WError WObject::InitInstancing(uint32_t maxInstances) {
	DestroyInstancingResources();

	float fExactWidth = sqrtf((float)maxInstances * 4.0f);
	uint32_t texWidth = 2;
	while (fExactWidth > texWidth)
		texWidth *= 2;

	// the instance buffer doesn't necessarily update every frame, so dont use W_IMAGE_CREATE_REWRITE_EVERY_FRAME
	float* texData = new float[texWidth * texWidth * 4];
	m_instanceTexture = m_app->ImageManager->CreateImage(texData, texWidth, texWidth, VK_FORMAT_R32G32B32A32_SFLOAT, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_DYNAMIC);
	W_SAFE_DELETE_ARRAY(texData);

	if (!m_instanceTexture)
		return WError(W_OUTOFMEMORY);

	m_instancesDirty = true;
	m_maxInstances = maxInstances;

	return WError(W_SUCCEEDED);
}

void WObject::DestroyInstancingResources() {
	W_SAFE_REMOVEREF(m_instanceTexture);
	for (uint32_t i = 0; i < m_instanceV.size(); i++)
		delete m_instanceV[i];
	m_instanceV.clear();
}

WInstance* WObject::CreateInstance() {
	if (!m_instanceTexture || m_instanceV.size() >= m_maxInstances)
		return nullptr;

	WInstance* inst = new WInstance();
	m_instanceV.push_back(inst);
	m_instancesDirty = true;
	return inst;
}

WInstance* WObject::GetInstance(uint32_t index) const {
	if (index < m_instanceV.size())
		return m_instanceV[index];
	return nullptr;
}

void WObject::DeleteInstance(WInstance* instance) {
	for (uint32_t i = 0; i < m_instanceV.size(); i++) {
		if (m_instanceV[i] == instance) {
			delete m_instanceV[i];
			m_instanceV.erase(m_instanceV.begin() + i);
			m_instancesDirty = true;
		}
	}
}

void WObject::DeleteInstance(uint32_t index) {
	if (index < m_instanceV.size()) {
		delete m_instanceV[index];
		m_instanceV.erase(m_instanceV.begin() + index);
		m_instancesDirty = true;
	}
}

uint32_t WObject::GetInstancesCount() const {
	return (uint32_t)m_instanceV.size();
}

void WObject::_UpdateInstanceBuffer() {
	if (m_instanceTexture && m_instanceV.size()) { //update the instance buffer
		bool bReconstruct = m_instancesDirty;
		for (uint32_t i = 0; i < m_instanceV.size(); i++)
			if (m_instanceV[i]->UpdateLocals())
				bReconstruct = true;
		if (bReconstruct) {
			void* pData;
			WError ret = m_instanceTexture->MapPixels(&pData, W_MAP_WRITE);
			if (ret) {
				for (uint32_t i = 0; i < m_instanceV.size(); i++) {
					WMatrix m = m_instanceV[i]->m_worldM;
					memcpy(&((char*)pData)[i * sizeof(WMatrix)], &m, sizeof(WMatrix) - sizeof(float));
				}
				m_instanceTexture->UnmapPixels();
			}
		}
		m_instancesDirty = false;
	}
}

WGeometry* WObject::GetGeometry() const {
	return m_geometry;
}

WAnimation* WObject::GetAnimation() const {
	return m_animation;
}

bool WObject::Hidden() const {
	return m_hidden;
}

void WObject::Show() {
	m_hidden = false;
}

void WObject::Hide() {
	m_hidden = true;
}

void WObject::EnableFrustumCulling() {
	m_bFrustumCull = true;
}

void WObject::DisableFrustumCulling() {
	m_bFrustumCull = false;
}

bool WObject::InCameraView(class WCamera* const cam) {
	WMatrix worldM = GetWorldMatrix();
	WVector3 min = WVec3TransformCoord(m_geometry->GetMinPoint(), worldM);
	WVector3 max = WVec3TransformCoord(m_geometry->GetMaxPoint(), worldM);
	WVector3 pos = (max + min) / 2.0f;
	WVector3 size = (max - min) / 2.0f;
	return cam->CheckBoxInFrustum(pos, size);
}

WVector3 WObject::GetScale() const {
	return m_scale;
}

void WObject::Scale(WVector3 scale) {
	m_bAltered = true;
	m_scale = scale;
}

WMatrix WObject::GetWorldMatrix() {
	UpdateLocals();
	return m_WorldM;
}

bool WObject::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		m_WorldM = ComputeTransformation();

		//scale matrix
		m_WorldM = WScalingMatrix(m_scale) * m_WorldM;

		return true;
	}

	return false;
}

void WObject::OnStateChange(STATE_CHANGE_TYPE type) { //virtual method of the orientation device
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}

WError WObject::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (!Valid())
		return WError(W_NOTVALID);

	outputStream.write((char*)&m_hidden, sizeof(m_hidden));
	outputStream.write((char*)&m_bFrustumCull, sizeof(m_bFrustumCull));
	outputStream.write((char*)&m_scale, sizeof(m_scale));
	WVector3 pos = GetPosition();
	outputStream.write((char*)&pos, sizeof(pos));
	WQuaternion rot = GetRotation();
	outputStream.write((char*)&rot, sizeof(rot));
	m_maxInstances = m_instanceTexture ? m_maxInstances : 0;
	outputStream.write((char*)&m_maxInstances, sizeof(m_maxInstances));
	uint32_t numInstances = (uint32_t)m_instanceV.size();
	outputStream.write((char*)&numInstances, sizeof(numInstances));
	for (uint32_t i = 0; i < (uint32_t)m_instanceV.size(); i++) {
		outputStream.write((char*)&m_instanceV[i]->m_scale, sizeof(m_instanceV[i]->m_scale));
		pos = m_instanceV[i]->GetPosition();
		outputStream.write((char*)&pos, sizeof(pos));
		rot = m_instanceV[i]->GetRotation();
		outputStream.write((char*)&rot, sizeof(rot));
	}

	uint32_t numMaterials = (uint32_t)m_materialMap.size();
	outputStream.write((char*)&numMaterials, sizeof(uint));

	char tmpName[W_MAX_ASSET_NAME_SIZE];
	std::vector<WFileAsset*> dependencies({ m_geometry, m_animation });
	for (auto mat : m_materialsCollection->m_materials)
		dependencies.push_back(mat.first);

	for (uint32_t i = 0; i < dependencies.size(); i++) {
		if (dependencies[i])
			strcpy_s(tmpName, W_MAX_ASSET_NAME_SIZE, dependencies[i]->GetName().c_str());
		else
			strcpy_s(tmpName, W_MAX_ASSET_NAME_SIZE, "");
		outputStream.write(tmpName, W_MAX_ASSET_NAME_SIZE);
	}


	for (uint32_t i = 0; i < dependencies.size(); i++) {
		if (dependencies[i]) {
			WError err = file->SaveAsset(dependencies[i]);
			if (!err)
				return err;
		}
	}

	return WError(W_SUCCEEDED);
}

std::vector<void*> WObject::LoadArgs() {
	return std::vector<void*>();
}

WError WObject::LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix) {
	UNREFERENCED_PARAMETER(args);

	DestroyInstancingResources();

	inputStream.read((char*)&m_hidden, sizeof(m_hidden));
	inputStream.read((char*)&m_bFrustumCull, sizeof(m_bFrustumCull));
	inputStream.read((char*)&m_scale, sizeof(m_scale));
	WVector3 pos;
	inputStream.read((char*)&pos, sizeof(pos));
	SetPosition(pos);
	WQuaternion rot;
	inputStream.read((char*)&rot, sizeof(rot));
	SetAngle(rot);
	uint32_t maxInstances;
	inputStream.read((char*)&maxInstances, sizeof(maxInstances));
	uint32_t numInstances;
	inputStream.read((char*)&numInstances, sizeof(numInstances));

	if (maxInstances > 0) {
		WError err = InitInstancing(maxInstances);
		if (!err)
			return err;
		for (uint32_t i = 0; i < numInstances; i++) {
			WInstance* inst = CreateInstance();
			inputStream.read((char*)&inst->m_scale, sizeof(inst->m_scale));
			inputStream.read((char*)&pos, sizeof(pos));
			inputStream.read((char*)&rot, sizeof(rot));
			inst->SetPosition(pos);
			inst->SetAngle(rot);
		}
	}

	uint32_t numMaterials;
	inputStream.read((char*)&numMaterials, sizeof(uint));

	WError status(W_SUCCEEDED);
	std::vector<std::string> dependencies;
	for (uint32_t i = 0; i < numMaterials + 2; i++) {
		char dependencyName[W_MAX_ASSET_NAME_SIZE];
		inputStream.read(dependencyName, W_MAX_ASSET_NAME_SIZE);
		dependencies.push_back(std::string(dependencyName));
	}

	WGeometry* geometry = nullptr;
	WSkeleton* animation = nullptr;

	if (dependencies[0] != "" && status)
		status = file->LoadAsset<WGeometry>(dependencies[0], &geometry, WGeometry::LoadArgs(), ""); // never copy the geometry
	if (status)
		status = SetGeometry(geometry);
	W_SAFE_REMOVEREF(geometry);

	if (dependencies[1] != "" && status)
		status = file->LoadAsset<WSkeleton>(dependencies[1], &animation, WSkeleton::LoadArgs(), nameSuffix); // copy the skeleton (if required)
	if (status)
		SetAnimation(animation);
	W_SAFE_REMOVEREF(animation);

	if (status) {
		ClearEffects();
		for (uint32_t i = 2; i < dependencies.size() && status; i++) {
			WMaterial* mat;
			status = file->LoadAsset<WMaterial>(dependencies[i], &mat, WMaterial::LoadArgs(), nameSuffix); // copy the materials (if required)
			if (status) {
				_AddMaterial(mat);
			}
		}
	}

	if (!status)
		DestroyInstancingResources();

	return status;
}
