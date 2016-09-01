#include "WLight.h"
#include "../Cameras/WCamera.h"

WLightManager::WLightManager(Wasabi* const app) : WManager<WLight>(app) {
	m_defaultLight = nullptr;
}

WLightManager::~WLightManager() {
	W_SAFE_REMOVEREF(m_defaultLight);
}

std::string WLightManager::GetTypeName() const {
	return "Light";
}

WError WLightManager::Load() {
	m_defaultLight = new WLight(m_app);
	m_defaultLight->SetPosition(0, 1, 0);
	m_defaultLight->Point(0, 0, 0);
	return WError(W_SUCCEEDED);
}

WLight* WLightManager::GetDefaultLight() const {
	return m_defaultLight;
}

WLight::WLight(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_type = W_LIGHT_DIRECTIONAL;
	m_range = 50.0f;
	m_color = WColor(1, 1, 1);
	m_intensity = 1.0f;
	SetEmittingAngle(45.0f);
	m_app->LightManager->AddEntity(this);
}

WLight::~WLight() {
	m_app->LightManager->RemoveEntity(this);
}

std::string WLight::GetTypeName() const {
	return "Light";
}

bool WLight::Valid() const {
	return true;
}

void WLight::SetType(W_LIGHT_TYPE type) {
	m_type = type;
}

void WLight::SetColor(WColor col) {
	m_color = col;
}

void WLight::SetRange(float fRange) {
	m_range = fRange;
}

void WLight::SetIntensity(float fIntensity) {
	m_intensity = fIntensity;
}

void WLight::SetEmittingAngle(float fAngle) {
	float fRads = W_DEGTORAD(fAngle / 2.0f);
	m_tanPhi = tanf(fRads) * m_range;
	m_cosAngle = cosf(fRads);
}

W_LIGHT_TYPE WLight::GetType() const {
	return m_type;
}

WColor WLight::GetColor() const {
	return m_color;
}

float WLight::GetRange() const {
	return m_range;
}

float WLight::GetIntensity() const {
	return m_intensity;
}

float WLight::GetMinCosAngle() const {
	return m_cosAngle;
}

void WLight::Show() {
	m_hidden = false;
}

void WLight::Hide() {
	m_hidden = true;
}

bool WLight::Hidden() const {
	return m_hidden;
}

bool WLight::InCameraView(WCamera* cam) const {
	if (m_type == W_LIGHT_DIRECTIONAL)
		return true;
	// not the most efficient for point lights, but a good approximation
	return cam->CheckSphereInFrustum(GetPosition(), m_range);
}

WMatrix WLight::GetWorldMatrix() {
	UpdateLocals(); //return matrix without offsets
	return m_WorldM;
}

bool WLight::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		WVector3 _up = GetUVector();
		WVector3 _look = GetLVector();
		WVector3 _right = GetRVector();
		WVector3 _pos = GetPosition();

		//
		// the world matrix is the view matrix's inverse
		// so we build a normal view matrix and invert it
		//

		// build world matrix
		float x = -WVec3Dot(_right, _pos);
		float y = -WVec3Dot(_up, _pos);
		float z = -WVec3Dot(_look, _pos);
		(m_WorldM)(0, 0) = _right.x; (m_WorldM)(0, 1) = _up.x; (m_WorldM)(0, 2) = _look.x; (m_WorldM)(0, 3) = 0.0f;
		(m_WorldM)(1, 0) = _right.y; (m_WorldM)(1, 1) = _up.y; (m_WorldM)(1, 2) = _look.y; (m_WorldM)(1, 3) = 0.0f;
		(m_WorldM)(2, 0) = _right.z; (m_WorldM)(2, 1) = _up.z; (m_WorldM)(2, 2) = _look.z; (m_WorldM)(2, 3) = 0.0f;
		(m_WorldM)(3, 0) = x;        (m_WorldM)(3, 1) = y;     (m_WorldM)(3, 2) = z;       (m_WorldM)(3, 3) = 1.0f;
		m_WorldM = WMatrixInverse(m_WorldM);

		return true;
	}

	return false;
}

void WLight::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type);

	m_bAltered = true;
}
