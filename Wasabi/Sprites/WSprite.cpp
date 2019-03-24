#include "WSprite.h"
#include "../Core/WCore.h"
#include "../Renderers/WRenderer.h"
#include "../Images/WImage.h"
#include "../Materials/WEffect.h"
#include "../Materials/WMaterial.h"
#include "../Geometries/WGeometry.h"
#include "../Renderers/Common/WSpritesRenderStage.h"
#include "../WindowAndInput/WWindowAndInputComponent.h"

struct SpriteVertex {
	WVector2 pos, uv;
};

class SpriteGeometry : public WGeometry {
public:
	SpriteGeometry(Wasabi* const app) : WGeometry(app) {}

	virtual unsigned int GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(unsigned int index) const {
		return W_VERTEX_DESCRIPTION({
			W_VERTEX_ATTRIBUTE("pos2", 2),
			W_ATTRIBUTE_UV,
		});
	}
};

std::string WSpriteManager::GetTypeName() const {
	return "Sprite";
}

WSpriteManager::WSpriteManager(class Wasabi* const app) : WManager<WSprite>(app) {
	m_spriteFullscreenGeometry = nullptr;
}

WSpriteManager::~WSpriteManager() {
	W_SAFE_REMOVEREF(m_spriteFullscreenGeometry);
}

WError WSpriteManager::Load() {
	m_spriteFullscreenGeometry = CreateSpriteGeometry();

	if (!m_spriteFullscreenGeometry)
		return WError(W_OUTOFMEMORY);

	return WError(W_SUCCEEDED);
}

WGeometry* WSpriteManager::CreateSpriteGeometry() const {
	const unsigned int num_verts = 4;
	SpriteVertex vb[num_verts];
	vb[0].pos = WVector2(-1, -1);
	vb[0].uv = WVector2(0, 0);
	vb[1].pos = WVector2(1, -1);
	vb[1].uv = WVector2(1, 0);
	vb[2].pos = WVector2(-1, 1);
	vb[2].uv = WVector2(0, 1);
	vb[3].pos = WVector2(1, 1);
	vb[3].uv = WVector2(1, 1);

	SpriteGeometry* geometry = new SpriteGeometry(m_app);
	WError err = geometry->CreateFromData(vb, num_verts, nullptr, 0, true);
	if (!err) {
		geometry->RemoveReference();
		return nullptr;
	}

	return geometry;
}

std::string WSprite::GetTypeName() const {
	return "Sprite";
}

WSprite::WSprite(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_img = nullptr;
	m_geometry = nullptr;
	m_angle = 0.0f;
	m_pos = WVector2();
	m_size = WVector2();
	m_rotationCenter = WVector2();
	m_priority = 0;
	m_lastLoadFailed = false;
	m_geometryChanged = true;
	m_material = nullptr;

	app->SpriteManager->AddEntity(this);
}

WSprite::WSprite(Wasabi* const app, WMaterial* material, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_img = nullptr;
	m_geometry = nullptr;
	m_angle = 0.0f;
	m_pos = WVector2();
	m_size = WVector2();
	m_rotationCenter = WVector2();
	m_priority = 0;
	m_lastLoadFailed = false;
	m_geometryChanged = true;
	SetMaterial(material);

	app->SpriteManager->AddEntity(this);
}

WSprite::~WSprite() {
	W_SAFE_REMOVEREF(m_img);
	W_SAFE_REMOVEREF(m_geometry);

	m_app->SpriteManager->RemoveEntity(this);
}

WError WSprite::Load(bool bCreateMaterial) {
	m_lastLoadFailed = true;
	W_SAFE_REMOVEREF(m_geometry);
	m_geometry = m_app->SpriteManager->CreateSpriteGeometry();
	if (!m_geometry)
		return WError(W_OUTOFMEMORY);
	m_geometryChanged = true;
	m_lastLoadFailed = false;
	return WError(W_SUCCEEDED);
}

void WSprite::SetImage(WImage* img) {
	if (!m_geometry && !m_lastLoadFailed)
		Load();

	W_SAFE_REMOVEREF(m_img);

	m_img = img;

	if (img) {
		SetSize(WVector2(img->GetWidth(), img->GetHeight()));
		img->AddReference();
	}
}

void WSprite::SetMaterial(WMaterial* mat) {
	if (mat) { // don't accept nullptr materials
		W_SAFE_REMOVEREF(m_material);
		m_material = mat;
		mat->AddReference();
	}
}

WMaterial* WSprite::GetMaterial() const {
	return m_material;
}

void WSprite::SetPosition(WVector2 pos) {
	m_pos = pos;
	m_geometryChanged = true;
}

void WSprite::Move(float units) {
	m_pos += WVec2TransformNormal(WVector2(1.0f, 0.0f), WRotationMatrixZ(m_angle)) * units;
	m_geometryChanged = true;
}

void WSprite::SetSize(WVector2 size) {
	m_size = size;
	m_geometryChanged = true;
}

void WSprite::SetAngle(float fAngle) {
	m_angle = W_DEGTORAD(fAngle);
	m_geometryChanged = true;
}

void WSprite::Rotate(float fAngle) {
	m_angle += W_DEGTORAD(fAngle);
	m_geometryChanged = true;
}

void WSprite::SetRotationCenter(float x, float y) {
	m_rotationCenter = WVector2(x, y);
	m_geometryChanged = true;
}

void WSprite::SetRotationCenter(WVector2 center) {
	m_rotationCenter = center;
	m_geometryChanged = true;
}

void WSprite::SetPriority(unsigned int priority) {
	m_priority = priority;
	m_geometryChanged = true;
}

void WSprite::Render(WRenderTarget* rt) {
	WVector2 screenDimensions = WVector2(m_app->WindowAndInputComponent->GetWindowWidth(),
										 m_app->WindowAndInputComponent->GetWindowHeight());
	if (!m_geometry && !m_lastLoadFailed)
		Load();

	if (!m_hidden && Valid()) {
		WGeometry* geometry = m_geometry;

		if (WVec2LengthSq(m_pos) < 0.1f && WVec2LengthSq(m_size - screenDimensions) < 0.1f && fabs(m_angle) < 0.001f) {
			geometry = m_app->SpriteManager->m_spriteFullscreenGeometry;
		} else {
			if (m_geometryChanged) {
				SpriteVertex* vb;
				geometry->MapVertexBuffer((void**)&vb);

				WVector2 rc = m_rotationCenter + m_pos;

				vb[0].pos = m_pos - rc;
				vb[1].pos = m_pos + WVector2(m_size.x, 0) - rc;
				vb[2].pos = m_pos + WVector2(0, m_size.y) - rc;
				vb[3].pos = m_pos + WVector2(m_size.x, m_size.y) - rc;

				WMatrix rotMtx = WRotationMatrixZ(m_angle) * WTranslationMatrix(rc.x, rc.y, 0);
				for (int i = 0; i < 4; i++) {
					WVector3 pos = WVector3(vb[i].pos.x, vb[i].pos.y, 0.0f);
					pos = WVec3TransformCoord(pos, rotMtx);
					vb[i].pos = WVector2(pos.x, pos.y);
					vb[i].pos = vb[i].pos * 2.0f / screenDimensions - WVector2(1, 1);
				}
				m_geometryChanged = false;
			}

			geometry->UnmapVertexBuffer();
		}

		geometry->Draw(rt);
	}
}

void WSprite::Show() {
	m_hidden = false;
}

void WSprite::Hide() {
	m_hidden = true;
}

bool WSprite::Hidden() const {
	return m_hidden;
}

float WSprite::GetAngle() const {
	return W_RADTODEG(m_angle);
}

WVector2 WSprite::GetPosition() const {
	return m_pos;
}

WVector2 WSprite::GetSize() const {
	return m_size;
}

unsigned int WSprite::GetPriority() const {
	return m_priority;
}


bool WSprite::Valid() const {
	return ((!m_material || m_material->Valid()) && m_geometry && m_geometry->Valid()) && m_size.x > 0.0f && m_size.y >= 0.0f;
}
