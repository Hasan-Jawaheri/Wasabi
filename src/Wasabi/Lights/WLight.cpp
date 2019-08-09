#include "Lights/WLight.h"
#include "Cameras/WCamera.h"

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
	m_defaultLight = new WDirectionalLight(m_app);
	m_defaultLight->SetPosition(0, 1, 0);
	m_defaultLight->Point(0, 0, 0);
	return WError(W_SUCCEEDED);
}

WLight* WLightManager::GetDefaultLight() const {
	return m_defaultLight;
}

WLight::WLight(Wasabi* const app, W_LIGHT_TYPE type, unsigned int ID) : WFileAsset(app, ID) {
	m_hidden = false;
	m_type = type;
	m_range = 50.0f;
	m_color = WColor(1, 1, 1);
	m_intensity = 1.0f;
	SetEmittingAngle(45.0f);
	m_app->LightManager->AddEntity(this);
}

WLight::~WLight() {
	m_app->LightManager->RemoveEntity(this);
}

std::string WLight::_GetTypeName() {
	return "Light";
}

std::string WLight::GetTypeName() const {
	return _GetTypeName();
}

bool WLight::Valid() const {
	return true;
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
		m_WorldM = ComputeTransformation();
		return true;
	}

	return false;
}

void WLight::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type);

	m_bAltered = true;
}

WError WLight::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (!Valid())
		return WError(W_NOTVALID);

	WVector3 pos = GetPosition();
	WQuaternion rot = GetRotation();
	outputStream.write((char*)&m_type, sizeof(m_type));
	outputStream.write((char*)&m_hidden, sizeof(m_hidden));
	outputStream.write((char*)&m_color, sizeof(m_color));
	outputStream.write((char*)&m_range, sizeof(m_range));
	outputStream.write((char*)&m_intensity, sizeof(m_intensity));
	outputStream.write((char*)&m_cosAngle, sizeof(m_cosAngle));
	outputStream.write((char*)&pos, sizeof(pos));
	outputStream.write((char*)&rot, sizeof(rot));

	return WError(W_SUCCEEDED);
}

std::vector<void*> WLight::LoadArgs() {
	return std::vector<void*>();
}

WError WLight::LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args) {
	WVector3 pos;
	WQuaternion rot;
	inputStream.read((char*)&m_type, sizeof(m_type));
	inputStream.read((char*)&m_hidden, sizeof(m_hidden));
	inputStream.read((char*)&m_color, sizeof(m_color));
	inputStream.read((char*)&m_range, sizeof(m_range));
	inputStream.read((char*)&m_intensity, sizeof(m_intensity));
	inputStream.read((char*)&m_cosAngle, sizeof(m_cosAngle));
	inputStream.read((char*)&pos, sizeof(pos));
	inputStream.read((char*)&rot, sizeof(rot));

	SetPosition(pos);
	SetAngle(rot);

	// this is necessary because we changed the m_type (this will allow e.g. lights renderer to reconfigure stuff for this light)
	m_app->LightManager->RemoveEntity(this);
	m_app->LightManager->AddEntity(this);

	return WError(W_SUCCEEDED);
}
