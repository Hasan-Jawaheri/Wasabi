#include "Wasabi/Sprites/WSprite.h"
#include "Wasabi/Core/WCore.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WImage.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Geometries/WGeometry.h"
#include "Wasabi/Renderers/Common/WSpritesRenderStage.h"
#include "Wasabi/WindowAndInput/WWindowAndInputComponent.h"

struct SpriteVertex {
	WVector2 pos, uv;
};

class SpriteGeometry : public WGeometry {
public:
	SpriteGeometry(Wasabi* const app) : WGeometry(app) {}

	virtual uint32_t GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(uint32_t index) const {
		UNREFERENCED_PARAMETER(index);
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
	vector<uint8_t> code {
		#include "Shaders/sprite.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

WSpritePS::WSpritePS(Wasabi* const app) : WShader(app) {}
void WSpritePS::Load(bool bSaveData) {
	m_desc.type = W_FRAGMENT_SHADER;
	m_desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 0, "uboPerSprite", {
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "alpha"),
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 1, "diffuseTexture"),
	};
	vector<uint8_t> code {
		#include "Shaders/sprite.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
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
	m_spriteFullscreenGeometry->SetName("SpriteFullscreenGeometry");
	m_app->FileManager->AddDefaultAsset(m_spriteFullscreenGeometry->GetName(), m_spriteFullscreenGeometry);

	if (!m_spriteFullscreenGeometry)
		return WError(W_OUTOFMEMORY);

	m_spriteVertexShader = new WSpriteVS(m_app);
	m_spriteVertexShader->SetName("SpriteDefaultVS");
	m_app->FileManager->AddDefaultAsset(m_spriteVertexShader->GetName(), m_spriteVertexShader);
	m_spriteVertexShader->Load();

	m_spritePixelShader = new WSpritePS(m_app);
	m_spritePixelShader->SetName("SpriteDefaultPS");
	m_app->FileManager->AddDefaultAsset(m_spritePixelShader->GetName(), m_spritePixelShader);
	m_spritePixelShader->Load();

	return WError(W_SUCCEEDED);
}

WError WSpriteManager::Resize(uint32_t width, uint32_t height) {
	UNREFERENCED_PARAMETER(width);
	UNREFERENCED_PARAMETER(height);

	uint32_t numSprites = GetEntitiesCount();
	for (uint32_t i = 0; i < numSprites; i++)
		GetEntityByIndex(i)->m_geometryChanged = true;
	return WError(W_SUCCEEDED);
}

WSprite* WSpriteManager::CreateSprite(WImage* img, uint32_t ID) const {
	return CreateSprite(nullptr, 0, img, ID);
}

WSprite* WSpriteManager::CreateSprite(WEffect* fx, uint32_t bindingSet, WImage* img, uint32_t ID) const {
	WGeometry* geometry = CreateSpriteGeometry();
	if (!geometry)
		return nullptr;

	WSprite* sprite = new WSprite(m_app, fx, bindingSet, ID);
	WMaterialCollection mats = sprite->GetMaterials();
	sprite->m_geometry = geometry;
	if (img) {
		mats.SetTexture("diffuseTexture", img);
		sprite->SetSize(img);
	}
	mats.SetVariable<float>("alpha", 1.0f);
	return sprite;
}

WGeometry* WSpriteManager::CreateSpriteGeometry() const {
	const uint32_t num_verts = 4;
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
		rs.lineWidth = 1.0f;
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

std::string WSprite::_GetTypeName() {
	return "Sprite";
}

std::string WSprite::GetTypeName() const {
	return _GetTypeName();
}

void WSprite::SetID(uint32_t newID) {
	m_app->SpriteManager->RemoveEntity(this);
	m_ID = newID;
	m_app->SpriteManager->AddEntity(this);
}

void WSprite::SetName(std::string newName) {
	m_name = newName;
	m_app->SpriteManager->OnEntityNameChanged(this, newName);
}

WSprite::WSprite(Wasabi* const app, uint32_t ID) : WSprite(app, nullptr, 0, ID) {}
WSprite::WSprite(Wasabi* const app, WEffect* fx, uint32_t bindingSet, uint32_t ID) : WBase(app, ID) {
	m_hidden = false;
	m_geometry = nullptr;
	m_angle = 0.0f;
	m_pos = WVector2();
	m_size = WVector2();
	m_rotationCenter = WVector2();
	m_priority = 0;
	m_geometryChanged = true;

	if (fx)
		AddEffect(fx, bindingSet);

	app->SpriteManager->AddEntity(this);
}

WSprite::~WSprite() {
	W_SAFE_REMOVEREF(m_geometry);

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
	SetSize(WVector2((float)image->GetWidth(), (float)image->GetHeight()));
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

void WSprite::SetPriority(uint32_t priority) {
	m_priority = priority;
	m_geometryChanged = true;
}

bool WSprite::WillRender(WRenderTarget* rt) {
	UNREFERENCED_PARAMETER(rt);
	return !m_hidden && Valid();
}

void WSprite::Render(WRenderTarget* rt) {
	WVector2 screenDimensions = WVector2((float)m_app->WindowAndInputComponent->GetWindowWidth(),
										 (float)m_app->WindowAndInputComponent->GetWindowHeight());
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

uint32_t WSprite::GetPriority() const {
	return m_priority;
}

bool WSprite::Valid() const {
	return (m_geometry && m_geometry->Valid()) && m_size.x > 0.0f && m_size.y >= 0.0f;
}
