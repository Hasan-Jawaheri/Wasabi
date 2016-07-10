#include "WLight.h"

WLightManager::WLightManager(Wasabi* const app) : WManager<WLight>(app) {

}

WLightManager::~WLightManager() {

}

std::string WLightManager::GetTypeName() const {
	return "Light";
}

WError WLightManager::Load() {

}

WLight::WLight(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;

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

void WLight::Show() {
	m_hidden = false;
}

void WLight::Hide() {
	m_hidden = true;
}

bool WLight::Hidden() const {
	return m_hidden;
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
