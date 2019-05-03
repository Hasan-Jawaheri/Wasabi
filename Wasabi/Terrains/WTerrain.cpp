#include "WTerrain.h"
#include "../Cameras/WCamera.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"
#include "../Geometries/WGeometry.h"
#include "../Materials/WMaterial.h"

WTerrainManager::WTerrainManager(class Wasabi* const app)
	: WManager<WTerrain>(app) {
}

WTerrainManager::~WTerrainManager() {
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

	m_blockGeometry = nullptr;

	app->TerrainManager->AddEntity(this);
}

WTerrain::~WTerrain() {
	_DestroyResources();

	m_app->TerrainManager->RemoveEntity(this);
}

std::string WTerrain::_GetTypeName() {
	return "Terrain";
}

std::string WTerrain::GetTypeName() const {
	return _GetTypeName();
}

void WTerrain::_DestroyResources() {
	W_SAFE_REMOVEREF(m_blockGeometry);
}

bool WTerrain::Valid() const {
	return m_blockGeometry != nullptr;
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

WError WTerrain::Create(unsigned int N, float size) {
	if (N > 1 && (N & (N - 1)) == N) // check power of 2
		return WError(W_INVALIDPARAM);
	if (size <= 0)
		return WError(W_INVALIDPARAM);
	_DestroyResources();

	m_blockGeometry = new WGeometry(m_app);
	WError err = m_blockGeometry->CreatePlain(N * size, N - 2, N - 2);
	if (err) {

	}

	return err;
}

bool WTerrain::WillRender(WRenderTarget* rt) {
	if (Valid() && !m_hidden) {
		/*WCamera* cam = rt->GetCamera();
		WMatrix worldM = GetWorldMatrix();
		if (m_bFrustumCull) {
			if (!InCameraView(cam))
				return false;
		}*/
		return true;
	}
	return false;
}

void WTerrain::Render(class WRenderTarget* const rt, WMaterial* material) {
	if (material) {
		WMatrix worldM = GetWorldMatrix();
		material->SetVariableMatrix("worldMatrix", worldM);
		material->Bind(rt);
	}

	m_blockGeometry->Draw(rt);
}

WMatrix WTerrain::GetWorldMatrix() {
	UpdateLocals();
	return m_WorldM;
}

bool WTerrain::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		m_WorldM = ComputeTransformation();

		return true;
	}

	return false;
}

void WTerrain::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}
