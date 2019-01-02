#include "WObject.h"
#include "../Core/WCore.h"
#include "../Core/WUtilities.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"
#include "../Cameras/WCamera.h"
#include "../Renderers/WRenderer.h"
#include "../Materials/WMaterial.h"
#include "../Materials/WEffect.h"
#include "../Geometries/WGeometry.h"
#include "../Windows/WWindowComponent.h"
#include "../Animations/WAnimation.h"

std::string WObjectManager::GetTypeName() const {
	return "Object";
}

WObjectManager::WObjectManager(class Wasabi* const app) : WManager<WObject>(app) {
}

void WObjectManager::Render(WRenderTarget* rt) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			m_entities[i][j]->Render(rt);
		}
	}
}

WError WObjectManager::Load() {
	return WError(W_SUCCEEDED);
}

WObject* WObjectManager::PickObject(int x, int y, bool bAnyHit, unsigned int iObjStartID, unsigned int iObjEndID,
									WVector3* _pt, WVector2* uv, unsigned int* faceIndex) const {
	struct pickStruct {
		WObject* obj;
		WVector3 pos;
		WVector2 uv;
		unsigned int face;
	};
	vector<pickStruct> pickedObjects;

	WCamera* cam = m_app->Renderer->GetDefaultRenderTarget()->GetCamera();
	if (!cam)
		return nullptr;

	int Width = m_app->WindowComponent->GetWindowWidth();
	int Height = m_app->WindowComponent->GetWindowHeight();
	cam->Render(Width, Height);

	WMatrix P = cam->GetProjectionMatrix();
	WMatrix V = cam->GetViewMatrix();
	WMatrix inverseV = WMatrixInverse(V);

	// Compute picking ray in view space.
	float vx = (+2.0f*x / Width  - 1.0f) / P(0, 0);
	float vy = (+2.0f*y / Height - 1.0f) / P(1, 1);

	WVector3 pos(0.0f, 0.0f, 0.0f);
	WVector3 dir(vx, vy, 1.0f);

	pos = WVec3TransformCoord(pos, inverseV);
	dir = WVec3TransformNormal(dir, inverseV);

	for (unsigned int j = 0; j < W_HASHTABLESIZE; j++) {
		for (unsigned int i = m_entities[j].size() - 1; i >= 0 && i != UINT_MAX; i--) {
			if (((m_entities[j][i]->GetID() >= iObjStartID && m_entities[j][i]->GetID() <= iObjEndID) ||
				(!iObjStartID && !iObjEndID)) && m_entities[j][i]->Valid()) {
				if (m_entities[j][i]->Hidden())
					continue;

				unsigned int hit = 0;
				WGeometry* temp = m_entities[j][i]->GetGeometry();
				if (temp) {
					//these calculations are per-subset
					WMatrix inverseW = WMatrixInverse(m_entities[j][i]->GetWorldMatrix());

					WVector3 subsetPos = WVec3TransformCoord(pos, inverseW);
					WVector3 subsetDir = WVec3TransformNormal(dir, inverseW);

					WVector3 boxPos = (temp->GetMaxPoint() + temp->GetMinPoint()) / 2.0f;
					WVector3 boxSize = (temp->GetMaxPoint() - temp->GetMinPoint()) / 2.0f;

					pickStruct p;
					p.obj = m_entities[j][i];
					if (WUtil::RayIntersectBox(boxSize, subsetPos, subsetDir, boxPos)) {
						WVector3 pt;
						bool b = temp->Intersect(subsetPos, subsetDir, &pt, &p.uv, &p.face);
						WMatrix m = m_entities[j][i]->GetWorldMatrix();
						p.pos.x = (pt.x * m(0, 0)) + (pt.y * m(1, 0)) + (pt.z * m(2, 0)) + (1 * m(3, 0));
						p.pos.y = (pt.x * m(0, 1)) + (pt.y * m(1, 1)) + (pt.z * m(2, 1)) + (1 * m(3, 1));
						p.pos.z = (pt.x * m(0, 2)) + (pt.y * m(1, 2)) + (pt.z * m(2, 2)) + (1 * m(3, 2));

						if (b) {
							if (bAnyHit) {
								if (faceIndex) *faceIndex = p.face;
								if (_pt) *_pt = pt;
								if (uv) *uv = p.uv;
								return m_entities[j][i];
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

	unsigned int nearest = 0;
	float distance = FLT_MAX;
	for (unsigned int i = 0; i < pickedObjects.size(); i++) {
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

void WInstance::ScaleX(float scale) {
	m_scale.x = scale;
	m_bAltered = true;
}

void WInstance::ScaleY(float scale) {
	m_scale.y = scale;
	m_bAltered = true;
}

void WInstance::ScaleZ(float scale) {
	m_scale.z = scale;
	m_bAltered = true;
}

WVector3 WInstance::GetScale() const {
	return m_scale;
}

float WInstance::GetScaleX() const {
	return m_scale.x;
}

float WInstance::GetScaleY() const {
	return m_scale.y;
}

float WInstance::GetScaleZ() const {
	return m_scale.z;
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
		(m_worldM)(0, 0) = _right.x; (m_worldM)(0, 1) = _up.x; (m_worldM)(0, 2) = _look.x; (m_worldM)(0, 3) = 0.0f;
		(m_worldM)(1, 0) = _right.y; (m_worldM)(1, 1) = _up.y; (m_worldM)(1, 2) = _look.y; (m_worldM)(1, 3) = 0.0f;
		(m_worldM)(2, 0) = _right.z; (m_worldM)(2, 1) = _up.z; (m_worldM)(2, 2) = _look.z; (m_worldM)(2, 3) = 0.0f;
		(m_worldM)(3, 0) = x;        (m_worldM)(3, 1) = y;     (m_worldM)(3, 2) = z;       (m_worldM)(3, 3) = 1.0f;
		m_worldM = WMatrixInverse(m_worldM);

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

WObject::WObject(Wasabi* const app, unsigned int ID) : WBase(app, ID), m_instanceV(0) {
	m_geometry = nullptr;
	m_material = app->Renderer->CreateDefaultMaterial();
	m_animation = nullptr;

	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;

	m_WorldM = WMatrix();
	m_scale = WVector3(1.0f, 1.0f, 1.0f);

	m_instanceTexture = nullptr;

	app->ObjectManager->AddEntity(this);
}

WObject::~WObject() {
	W_SAFE_REMOVEREF(m_geometry);
	W_SAFE_REMOVEREF(m_material);
	W_SAFE_REMOVEREF(m_animation);

	VkDevice device = m_app->GetVulkanDevice();
	W_SAFE_REMOVEREF(m_instanceTexture);
	for (int i = 0; i < m_instanceV.size(); i++)
		delete m_instanceV[i];
	m_instanceV.clear();

	m_app->ObjectManager->RemoveEntity(this);
}

std::string WObject::GetTypeName() const {
	return "Object";
}

bool WObject::Valid() const {
	if (m_geometry && m_material && m_geometry->Valid() && m_material->Valid())
		if (m_geometry->GetVertexDescriptionSize(0) == m_material->GetEffect()->GetInputLayoutSize(0))
			return true;

		return false;
}

void WObject::Render(WRenderTarget* rt) {
	if (Valid() && !m_hidden) {
		WCamera* cam = rt->GetCamera();
		WMatrix worldM = GetWorldMatrix();
		if (m_bFrustumCull) {
			WVector3 min = WVec3TransformCoord(m_geometry->GetMinPoint(), worldM);
			WVector3 max = WVec3TransformCoord(m_geometry->GetMaxPoint(), worldM);
			WVector3 pos = (max + min) / 2.0f;
			WVector3 size = (max - min) / 2.0f;
			if (!cam->CheckBoxInFrustum(pos, size))
				return; //outside viewing frustum
		}

		_UpdateInstanceBuffer();

		m_material->SetVariableMatrix("gWorld", worldM);
		m_material->SetVariableMatrix("gProjection", cam->GetProjectionMatrix());
		m_material->SetVariableMatrix("gView", cam->GetViewMatrix());
		m_material->SetVariableVector3("gCamPos", cam->GetPosition());
		// animation variables
		bool is_animated = m_animation && m_animation->Valid() && m_geometry->IsRigged();
		bool is_instanced = m_instanceV.size() > 0;
		m_material->SetVariableInt("gAnimation", is_animated ? 1 : 0);
		m_material->SetVariableInt("gInstancing", is_instanced ? 1 : 0);
		if (is_animated) {
			WImage* animTex = m_animation->GetTexture();
			m_material->SetVariableInt("gAnimationTextureWidth", animTex->GetWidth());
			m_material->SetAnimationTexture(animTex);
		}
		// instancing variables
		if (is_instanced) {
			m_material->SetVariableInt("gInstanceTextureWidth", m_instanceTexture->GetWidth());
			m_material->SetInstancingTexture(m_instanceTexture);
		}

		WError err;
		// bind the pipeline
		err = m_material->Bind(rt, is_animated ? 2 : 1);

		// bind the rest of the buffers and draw
		err = m_geometry->Draw(rt, -1, fmax(m_instanceV.size(), 1), is_animated);
	}
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

WError WObject::SetMaterial(class WMaterial* material) {
	if (m_material)
		m_material->RemoveReference();

	m_material = material;
	if (material) {
		m_material->AddReference();
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

WError WObject::InitInstancing(unsigned int maxInstances) {
	W_SAFE_REMOVEREF(m_instanceTexture);
	for (int i = 0; i < m_instanceV.size(); i++)
		delete m_instanceV[i];
	m_instanceV.clear();

	float fExactWidth = sqrtf(maxInstances * 4);
	unsigned int texWidth = 2;
	while (fExactWidth > texWidth)
		texWidth *= 2;

	float* texData = new float[texWidth * texWidth * 4];
	m_instanceTexture = new WImage(m_app);
	WError ret = m_instanceTexture->CreateFromPixelsArray(texData, texWidth, texWidth, true);
	W_SAFE_DELETE_ARRAY(texData);

	if (!ret) {
		W_SAFE_REMOVEREF(m_instanceTexture);
		return ret;
	}

	m_instancesDirty = true;
	m_maxInstances = maxInstances;

	return WError(W_SUCCEEDED);
}

WInstance* WObject::CreateInstance() {
	if (!m_instanceTexture || m_instanceV.size() >= m_maxInstances)
		return nullptr;

	WInstance* inst = new WInstance();
	m_instanceV.push_back(inst);
	m_instancesDirty = true;
	return inst;
}

WInstance* WObject::GetInstance(unsigned int index) const {
	if (index < m_instanceV.size())
		return m_instanceV[index];
	return nullptr;
}

void WObject::DeleteInstance(WInstance* instance) {
	for (uint i = 0; i < m_instanceV.size(); i++) {
		if (m_instanceV[i] == instance) {
			delete m_instanceV[i];
			m_instanceV.erase(m_instanceV.begin() + i);
			m_instancesDirty = true;
		}
	}
}

void WObject::DeleteInstance(unsigned int index) {
	if (index < m_instanceV.size()) {
		delete m_instanceV[index];
		m_instanceV.erase(m_instanceV.begin() + index);
		m_instancesDirty = true;
	}
}

unsigned int WObject::GetInstancesCount() const {
	return m_instanceV.size();
}

void WObject::_UpdateInstanceBuffer() {
	if (m_instanceTexture && m_instanceV.size()) { //update the instance buffer
		bool bReconstruct = m_instancesDirty;
		for (unsigned int i = 0; i < m_instanceV.size(); i++)
			if (m_instanceV[i]->UpdateLocals())
				bReconstruct = true;
		if (bReconstruct) {
			void* pData;
			WError ret = m_instanceTexture->MapPixels(&pData);
			if (ret) {
				for (unsigned int i = 0; i < m_instanceV.size(); i++) {
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

WMaterial* WObject::GetMaterial() const {
	return m_material;
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

WVector3 WObject::GetScale() const {
	return m_scale;
}

float WObject::GetScaleX() const {
	return m_scale.x;
}

float WObject::GetScaleY() const {
	return m_scale.y;
}

float WObject::GetScaleZ() const {
	return m_scale.z;
}

void WObject::Scale(float x, float y, float z) {
	m_bAltered = true;
	m_scale = WVector3(x, y, z);
}

void WObject::Scale(WVector3 scale) {
	m_bAltered = true;
	m_scale = scale;
}

void WObject::ScaleX(float scale) {
	m_bAltered = true;
	m_scale.x = scale;
}

void WObject::ScaleY(float scale) {
	m_bAltered = true;
	m_scale.y = scale;
}

void WObject::ScaleZ(float scale) {
	m_bAltered = true;
	m_scale.z = scale;
}

WMatrix WObject::GetWorldMatrix() {
	UpdateLocals(WVector3(0, 0, 0)); //return matrix without offsets
	return m_WorldM;
}

bool WObject::UpdateLocals(WVector3 offset) {
	if (m_bAltered) {
		m_bAltered = false;
		WVector3 _up = GetUVector();
		WVector3 _look = GetLVector();
		WVector3 _right = GetRVector();
		WVector3 _pos = GetPosition();

		//
		//the world matrix is the view matrix's inverse
		//so we build a normal view matrix and invert it
		//

		//build world matrix
		float x = -WVec3Dot(_right, _pos) - offset.x; //apply offset
		float y = -WVec3Dot(_up, _pos) - offset.y; //apply offset
		float z = -WVec3Dot(_look, _pos) - offset.z; //apply offset
		(m_WorldM)(0, 0) = _right.x; (m_WorldM)(0, 1) = _up.x; (m_WorldM)(0, 2) = _look.x; (m_WorldM)(0, 3) = 0.0f;
		(m_WorldM)(1, 0) = _right.y; (m_WorldM)(1, 1) = _up.y; (m_WorldM)(1, 2) = _look.y; (m_WorldM)(1, 3) = 0.0f;
		(m_WorldM)(2, 0) = _right.z; (m_WorldM)(2, 1) = _up.z; (m_WorldM)(2, 2) = _look.z; (m_WorldM)(2, 3) = 0.0f;
		(m_WorldM)(3, 0) = x;        (m_WorldM)(3, 1) = y;     (m_WorldM)(3, 2) = z;       (m_WorldM)(3, 3) = 1.0f;
		m_WorldM = WMatrixInverse(m_WorldM);

		//scale matrix
		m_WorldM = WScalingMatrix(m_scale) * m_WorldM;

		return true;
	}

	return false;
}

void WObject::OnStateChange(STATE_CHANGE_TYPE type) //virtual method of the orientation device
{
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}

