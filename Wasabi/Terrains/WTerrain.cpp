#include "WTerrain.h"
#include "../Cameras/WCamera.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"

WTerrainManager::WTerrainManager(class Wasabi* const app)
	: WManager<WTerrain>(app) {
}

WTerrainManager::~WTerrainManager() {
}

void WTerrainManager::Render(WRenderTarget* rt) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			m_entities[i][j]->Render(rt);
		}
	}
}

std::string WTerrainManager::GetTypeName() const {
	return "Terrain";
}

WError WTerrainManager::Load() {
	return WError(W_SUCCEEDED);
}

WTerrain::WTerrain(class Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_bAltered = true;

	m_WorldM = WMatrix();

	app->TerrainManager->AddEntity(this);
}

WTerrain::~WTerrain() {
	_DestroyResources();

	m_app->TerrainManager->RemoveEntity(this);
}

std::string WTerrain::GetTypeName() const {
	return "Terrain";
}

void WTerrain::_DestroyResources() {
}

bool WTerrain::Valid() const {
	return false;
}

void WTerrain::Show() {
	m_hidden = false;
}

void WTerrain::Hide() {
	m_hidden = true;
}

bool WTerrain::Hidden() const {
	return m_hidden;
}

WError WTerrain::Create() {
	_DestroyResources();
	return WError(W_SUCCEEDED);
}

void WTerrain::Render(class WRenderTarget* const rt) {
	if (Valid() && !m_hidden) {
		WCamera* cam = rt->GetCamera();
		WMatrix worldM = GetWorldMatrix();
	}
}

WMatrix WTerrain::GetWorldMatrix() {
	UpdateLocals();
	return m_WorldM;
}

bool WTerrain::UpdateLocals() {
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
		float x = -WVec3Dot(_right, _pos); //apply offset
		float y = -WVec3Dot(_up, _pos); //apply offset
		float z = -WVec3Dot(_look, _pos); //apply offset
		(m_WorldM)(0, 0) = _right.x; (m_WorldM)(0, 1) = _up.x; (m_WorldM)(0, 2) = _look.x; (m_WorldM)(0, 3) = 0.0f;
		(m_WorldM)(1, 0) = _right.y; (m_WorldM)(1, 1) = _up.y; (m_WorldM)(1, 2) = _look.y; (m_WorldM)(1, 3) = 0.0f;
		(m_WorldM)(2, 0) = _right.z; (m_WorldM)(2, 1) = _up.z; (m_WorldM)(2, 2) = _look.z; (m_WorldM)(2, 3) = 0.0f;
		(m_WorldM)(3, 0) = x;        (m_WorldM)(3, 1) = y;     (m_WorldM)(3, 2) = z;       (m_WorldM)(3, 3) = 1.0f;
		m_WorldM = WMatrixInverse(m_WorldM);

		return true;
	}

	return false;
}

void WTerrain::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}
