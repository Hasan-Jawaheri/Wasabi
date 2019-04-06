#include "WSprite.h"
#include "../Core/WCore.h"
#include "../Renderers/WRenderer.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"
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

WSpriteVS::WSpriteVS(Wasabi* const app) : WShader(app) {}
void WSpriteVS::Load(bool bSaveData) {
	m_desc.type = W_VERTEX_SHADER;
	m_desc.input_layouts = { W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
	}) };
	vector<byte> code = {
		#include "Shaders/sprite.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
}

WSpritePS::WSpritePS(Wasabi* const app) : WShader(app) {}
void WSpritePS::Load(bool bSaveData) {
	m_desc.type = W_FRAGMENT_SHADER;
	m_desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 0, "uboPerSprite", {
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "alpha"),
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 1, "textureDiffuse"),
	};
	vector<byte> code = {
		#include "Shaders/sprite.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
}

std::string WSpriteManager::GetTypeName() const {
	return "Sprite";
}

WSpriteManager::WSpriteManager(class Wasabi* const app) : WManager<WSprite>(app) {
	m_spriteFullscreenGeometry = nullptr;
	m_spriteVertexShader = nullptr;
	m_spritePixelShader = nullptr;
}

WSpriteManager::~WSpriteManager() {
	W_SAFE_REMOVEREF(m_spriteFullscreenGeometry);
	W_SAFE_REMOVEREF(m_spriteVertexShader);
	W_SAFE_REMOVEREF(m_spritePixelShader);
}

WError WSpriteManager::Load() {
	m_spriteFullscreenGeometry = CreateSpriteGeometry();

	if (!m_spriteFullscreenGeometry)
		return WError(W_OUTOFMEMORY);

	m_spriteVertexShader = new WSpriteVS(m_app);
	m_spriteVertexShader->Load();

	m_spritePixelShader = new WSpritePS(m_app);
	m_spritePixelShader->Load();

	return WError(W_SUCCEEDED);
}

WError WSpriteManager::Resize(unsigned int width, unsigned int height) {
	uint numSprites = GetEntitiesCount();
	for (uint i = 0; i < numSprites; i++)
		GetEntityByIndex(i)->m_geometryChanged = true;
	return WError(W_SUCCEEDED);
}

WSprite* WSpriteManager::CreateSprite(WImage* img, unsigned int ID) const {
	WGeometry* geometry = CreateSpriteGeometry();
	if (!geometry)
		return nullptr;

	WSprite* sprite = new WSprite(m_app, ID);
	sprite->m_geometry = geometry;
	if (img) {
		sprite->GetMaterial()->SetTexture("diffuseTexture", img);
		sprite->SetSize(img);
	}
	WMaterial* mat = sprite->GetMaterial();
	if (mat) {
		mat->SetVariableFloat("alpha", 1.0f);
	}
	return sprite;
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
	WError err = geometry->CreateFromData(vb, num_verts, nullptr, 0, W_GEOMETRY_CREATE_VB_DYNAMIC);
	if (!err) {
		geometry->RemoveReference();
		return nullptr;
	}

	return geometry;
}

WEffect* WSpriteManager::CreateSpriteEffect(
	WRenderTarget* rt,
	WShader* ps,
	VkPipelineColorBlendAttachmentState bs,
	VkPipelineDepthStencilStateCreateInfo dss,
	VkPipelineRasterizationStateCreateInfo rs
) const {
	WEffect* spriteFX = new WEffect(m_app);
	WError err = spriteFX->BindShader(m_spriteVertexShader);
	if (!err) {
		spriteFX->RemoveReference();
		return nullptr;
	}
	if (!ps)
		ps = m_spritePixelShader;
	err = spriteFX->BindShader(ps);
	if (!err) {
		spriteFX->RemoveReference();
		return nullptr;
	}

	if (bs.colorWriteMask == 0) {
		bs.blendEnable = VK_TRUE;
		bs.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		bs.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		bs.colorBlendOp = VK_BLEND_OP_ADD;
		bs.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		bs.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		bs.alphaBlendOp = VK_BLEND_OP_ADD;
		bs.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}
	spriteFX->SetBlendingState(bs);

	if (dss.sType == 0) {
		dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		dss.depthTestEnable = VK_FALSE;
		dss.depthWriteEnable = VK_FALSE;
		dss.depthBoundsTestEnable = VK_FALSE;
		dss.stencilTestEnable = VK_FALSE;
		dss.front = dss.back;
	}
	spriteFX->SetDepthStencilState(dss);

	if (rs.sType == 0) {
		rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rs.polygonMode = VK_POLYGON_MODE_FILL; // Solid polygon mode
		rs.cullMode = VK_CULL_MODE_NONE; // Backface culling
		rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
	}
	spriteFX->SetRasterizationState(rs);

	spriteFX->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP); // we use a triangle strip with not index buffer

	err = spriteFX->BuildPipeline(rt ? rt : m_app->Renderer->GetRenderTarget(m_app->Renderer->GetSpritesRenderStageName()));

	if (!err) {
		spriteFX->RemoveReference();
		return nullptr;
	}

	return spriteFX;
}

WShader* WSpriteManager::GetSpriteVertexShader() const {
	return m_spriteVertexShader;
}

WShader* WSpriteManager::GetSpritePixelShader() const {
	return m_spritePixelShader;
}


std::string WSprite::GetTypeName() const {
	return "Sprite";
}

WSprite::WSprite(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_geometry = nullptr;
	m_angle = 0.0f;
	m_pos = WVector2();
	m_size = WVector2();
	m_rotationCenter = WVector2();
	m_priority = 0;
	m_geometryChanged = true;

	app->SpriteManager->AddEntity(this);
}

WSprite::~WSprite() {
	W_SAFE_REMOVEREF(m_geometry);

	ClearEffects();

	m_app->SpriteManager->RemoveEntity(this);
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

void WSprite::SetSize(WImage* image) {
	SetSize(WVector2(image->GetWidth(), image->GetHeight()));
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

bool WSprite::WillRender(WRenderTarget* rt) {
	return !m_hidden && Valid();
}

void WSprite::Render(WRenderTarget* rt) {
	WVector2 screenDimensions = WVector2(m_app->WindowAndInputComponent->GetWindowWidth(),
										 m_app->WindowAndInputComponent->GetWindowHeight());
	WGeometry* geometry = m_geometry;

	if (WVec2LengthSq(m_pos) < 0.1f && WVec2LengthSq(m_size - screenDimensions) < 0.1f && fabs(m_angle) < 0.001f) {
		geometry = m_app->SpriteManager->m_spriteFullscreenGeometry;
	} else {
		if (m_geometryChanged) {
			SpriteVertex* vb;
			geometry->MapVertexBuffer((void**)&vb, W_MAP_WRITE);

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

			geometry->UnmapVertexBuffer(false);

			m_geometryChanged = false;
		}
	}

	geometry->Draw(rt);
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
	return (m_geometry && m_geometry->Valid()) && m_size.x > 0.0f && m_size.y >= 0.0f;
}
