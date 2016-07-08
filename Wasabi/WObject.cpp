#include "WObject.h"

std::string WObjectManager::GetTypeName() const {
	return "Object";
}

WObjectManager::WObjectManager(class Wasabi* const app) : WManager<WObject>(app) {
}

void WObjectManager::Render(WCamera* const cam) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < _entities[i].size(); j++) {
			_entities[i][j]->Render(cam);
		}
	}
}

WObject::WObject(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_geometry = nullptr;
	m_material = app->Renderer->CreateDefaultMaterial();

	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;

	m_WorldM = WMatrix();
	m_fScaleX = m_fScaleY = m_fScaleZ = 100.0f;

	app->ObjectManager->AddEntity(this);
}

WObject::~WObject() {
	W_SAFE_REMOVEREF(m_geometry);
	W_SAFE_REMOVEREF(m_material);

	m_app->ObjectManager->RemoveEntity(this);
}

std::string WObject::GetTypeName() const {
	return "Object";
}

bool WObject::Valid() const {
	return true; // TODO: put actual implementation
}

void WObject::Render(WCamera* const cam) {
	if (m_geometry && m_material && m_geometry->Valid() && m_material->Valid() && !m_hidden) {
		if (m_geometry->GetVertexDescription().GetSize() != m_material->GetEffect()->GetInputLayout().GetSize())
			return;
		WMatrix worldM = GetWorldMatrix();
		if (m_bFrustumCull) {
			WMatrix wmtx = GetWorldMatrix();
			WVector3 min = WVec3TransformCoord(m_geometry->GetMinPoint(), wmtx);
			WVector3 max = WVec3TransformCoord(m_geometry->GetMaxPoint(), wmtx);
			WVector3 pos = (max + min) / 2.0f;
			WVector3 size = (max - min) / 2.0f;
			if (!cam->CheckBoxInFrustum(pos, size))
				return; //outside viewing frustum
		}

		m_material->SetVariableMatrix("gWorld", worldM);
		m_material->SetVariableMatrix("gProjection", cam->GetProjectionMatrix());
		m_material->SetVariableMatrix("gView", cam->GetViewMatrix());

		WError err;
		err = m_material->Bind();
		err = m_geometry->Draw();
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

WGeometry* WObject::GetGeometry() const {
	return m_geometry;
}

WMaterial* WObject::GetMaterial() const {
	return m_material;
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
	return WVector3(m_fScaleX, m_fScaleY, m_fScaleZ);
}

float WObject::GetScaleX() const {
	return m_fScaleX;
}

float WObject::GetScaleY() const {
	return m_fScaleY;
}

float WObject::GetScaleZ() const {
	return m_fScaleZ;
}

void WObject::Scale(float x, float y, float z) {
	m_bAltered = true;
	m_fScaleX = x;
	m_fScaleY = y;
	m_fScaleZ = z;
}

void WObject::Scale(WVector3 scale) {
	m_bAltered = true;
	m_fScaleX = scale.x;
	m_fScaleY = scale.y;
	m_fScaleZ = scale.z;
}

void WObject::ScaleX(float scale) {
	m_bAltered = true;
	m_fScaleX = scale;
}

void WObject::ScaleY(float scale) {
	m_bAltered = true;
	m_fScaleY = scale;
}

void WObject::ScaleZ(float scale) {
	m_bAltered = true;
	m_fScaleZ = scale;
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
		m_WorldM = WScalingMatrix(m_fScaleX / 100.0f, m_fScaleY / 100.0f, m_fScaleZ / 100.0f) * m_WorldM;

		return true;
	}

	return false;
}

void WObject::OnStateChange(STATE_CHANGE_TYPE type) //virtual method of the orientation device
{
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}

