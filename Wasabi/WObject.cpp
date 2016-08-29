#include "WObject.h"

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


WObject::WObject(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_geometry = nullptr;
	m_material = app->Renderer->CreateDefaultMaterial();
	m_animation = nullptr;

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
	W_SAFE_REMOVEREF(m_animation);

	m_app->ObjectManager->RemoveEntity(this);
}

std::string WObject::GetTypeName() const {
	return "Object";
}

bool WObject::Valid() const {
	return true; // TODO: put actual implementation
}

void WObject::Render(WRenderTarget* rt) {
	if (m_geometry && m_material && m_geometry->Valid() && m_material->Valid() && !m_hidden) {
		if (m_geometry->GetVertexDescription(0).GetSize() != m_material->GetEffect()->GetInputLayout(0).GetSize())
			return;
		WCamera* cam = rt->GetCamera();
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
		m_material->SetVariableVector3("gCamPos", cam->GetPosition());
		if (m_animation && m_animation->Valid()) {
			WImage* animTex = m_animation->GetTexture();
			m_material->SetVariableInt("gAnimation", 1);
			m_material->SetVariableInt("gAnimationTextureWidth", animTex->GetWidth());
			m_material->SetAnimationTexture(animTex);
		} else
			m_material->SetVariableInt("gAnimation", 0);

		WError err;
		err = m_material->Bind(rt);
		err = m_geometry->Draw(rt);
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

