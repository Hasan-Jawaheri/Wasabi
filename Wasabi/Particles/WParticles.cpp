#include "WParticles.h"
#include "../Cameras/WCamera.h"
#include "../Images/WRenderTarget.h"
#include "../Geometries/WGeometry.h"
#include "../Materials/WEffect.h"
#include "../Materials/WMaterial.h"

class WParticlesGeometry : public WGeometry {
	const W_VERTEX_DESCRIPTION m_desc = W_VERTEX_DESCRIPTION({ W_ATTRIBUTE_POSITION });

public:
	WParticlesGeometry(Wasabi* const app, unsigned int ID = 0) : WGeometry(app, ID) {}

	virtual unsigned int GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(unsigned int layout_index = 0) const {
		return m_desc;
	}
	virtual size_t GetVertexDescriptionSize(unsigned int layout_index = 0) const {
		return m_desc.GetSize();
	}
};

WParticlesManager::WParticlesManager(class Wasabi* const app) : WManager<WParticles>(app) {

}

WParticlesManager::~WParticlesManager() {

}

void WParticlesManager::Render(WRenderTarget* rt) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			m_entities[i][j]->Render(rt);
		}
	}
}

std::string WParticlesManager::GetTypeName() const {
	return "Particles";
}

WError WParticlesManager::Load() {
	return WError(W_SUCCEEDED);
}


WParticles::WParticles(class Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;

	m_WorldM = WMatrix();

	m_geometry = nullptr;
	m_material = nullptr;

	app->ParticlesManager->AddEntity(this);
}

WParticles::~WParticles() {
	_DestroyResources();

	m_app->ParticlesManager->RemoveEntity(this);
}

std::string WParticles::GetTypeName() const {
	return "Particles";
}

void WParticles::_DestroyResources() {
	W_SAFE_REMOVEREF(m_geometry);
	W_SAFE_REMOVEREF(m_material);
}

bool WParticles::Valid() const {
	return false;
}

void WParticles::Show() {
	m_hidden = false;
}

void WParticles::Hide() {
	m_hidden = true;
}

bool WParticles::Hidden() const {
	return m_hidden;
}

bool WParticles::InCameraView(class WCamera* cam) {
	return false;
	//WMatrix worldM = GetWorldMatrix();
	//WVector3 min = WVec3TransformCoord(m_geometry->GetMinPoint(), worldM);
	//WVector3 max = WVec3TransformCoord(m_geometry->GetMaxPoint(), worldM);
	//WVector3 pos = (max + min) / 2.0f;
	//WVector3 size = (max - min) / 2.0f;
	//return cam->CheckBoxInFrustum(pos, size);
}

WError WParticles::Create(unsigned int max_particles) {
	_DestroyResources();
	if (max_particles == 0)
		return WError(W_INVALIDPARAM);

	m_geometry = new WParticlesGeometry(m_app);
	m_material = new WMaterial(m_app);

	WParticlesVertex* vertices = new WParticlesVertex[max_particles];
	m_geometry->CreateFromData(vertices, max_particles, nullptr, 0, true);
	delete[] vertices;

	return WError(W_SUCCEEDED);
}

void WParticles::Render(class WRenderTarget* const rt) {
	if (Valid() && !m_hidden) {
		WCamera* cam = rt->GetCamera();
		WMatrix worldM = GetWorldMatrix();
		if (m_bFrustumCull) {
			if (!InCameraView(cam))
				return;
		}
	}
}

WMatrix WParticles::GetWorldMatrix() {
	UpdateLocals();
	return m_WorldM;
}

bool WParticles::UpdateLocals() {
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

void WParticles::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}

