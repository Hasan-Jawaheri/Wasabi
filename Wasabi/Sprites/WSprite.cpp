#include "WSprite.h"
#include "../Core/WCore.h"
#include "../Renderers/WRenderer.h"
#include "../Images/WImage.h"
#include "../Materials/WEffect.h"
#include "../Materials/WMaterial.h"
#include "../Geometries/WGeometry.h"
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

class SpriteVS : public WShader {
public:
	SpriteVS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.input_layouts = {W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // UV
		})};
		LoadCodeGLSL(
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(location = 0) in  vec2 inPos;\n"
			"layout(location = 1) in  vec2 inUV;\n"
			"layout(location = 0) out vec2 outUV;\n"
			""
			"void main() {\n"
			"	outUV = inUV;\n"
			"	gl_Position = vec4(inPos.xy, 0.0, 1.0);\n"
			"}\n"
		);
	}
};

class SpritePS : public WShader {
public:
	SpritePS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0,{
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 1, "alpha"),
			}),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
		};
		LoadCodeGLSL(
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(binding = 0) uniform UBO {\n"
			"	float alpha;\n"
			"} ubo;\n"
			"layout(binding = 1) uniform sampler2D sampler;\n"
			"layout(location = 0) in vec2 inUV;\n"
			"layout(location = 0) out vec4 outFragColor;\n"
			""
			"void main() {\n"
			"	vec4 c = texture(sampler, inUV);\n"
			"	outFragColor = vec4(c.rgb, c.a * ubo.alpha);\n"
			"}\n"
		);
	}
};

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
	m_spriteVertexShader = new SpriteVS(m_app);
	m_spriteVertexShader->Load();

	m_spritePixelShader = new SpritePS(m_app);
	m_spritePixelShader->Load();

	m_spriteFullscreenGeometry = CreateSpriteGeometry();

	if (!m_spriteFullscreenGeometry) {
		W_SAFE_REMOVEREF(m_spriteVertexShader);
		W_SAFE_REMOVEREF(m_spritePixelShader);
		return WError(W_OUTOFMEMORY);
	}

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

WEffect* WSpriteManager::CreateSpriteEffect(
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

	err = spriteFX->BuildPipeline(m_app->Renderer->GetDefaultRenderTarget());

	if (!err) {
		spriteFX->RemoveReference();
		return nullptr;
	}

	return spriteFX;
}

void WSpriteManager::Render(WRenderTarget* rt) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			m_entities[i][j]->Render(rt);
		}
	}
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
	m_img = nullptr;
	m_geometry = nullptr;
	m_material = nullptr;
	m_angle = 0.0f;
	m_pos = WVector2();
	m_size = WVector2();
	m_rotationCenter = WVector2();
	m_priority = 0;
	m_alpha = 1.0f;
	m_lastLoadFailed = false;
	m_geometry_changed = true;

	app->SpriteManager->AddEntity(this);
}

WSprite::~WSprite() {
	W_SAFE_REMOVEREF(m_img);
	W_SAFE_REMOVEREF(m_geometry);
	W_SAFE_REMOVEREF(m_material);

	m_app->SpriteManager->RemoveEntity(this);
}

WError WSprite::Load() {
	m_lastLoadFailed = true;
	W_SAFE_REMOVEREF(m_geometry);
	m_geometry = m_app->SpriteManager->CreateSpriteGeometry();
	if (!m_geometry)
		return WError(W_OUTOFMEMORY);
	m_geometry_changed = true;

	if (!m_material) {
		WEffect* fx = m_app->SpriteManager->CreateSpriteEffect();
		m_material = new WMaterial(m_app);
		WError err = m_material->SetEffect(fx);
		W_SAFE_REMOVEREF(fx);
		if (!err) {
			W_SAFE_REMOVEREF(m_material);
			return err;
		}
		SetAlpha(1);
	}

	m_lastLoadFailed = false;
	return WError(W_SUCCEEDED);
}

void WSprite::SetImage(WImage* img) {
	if (!m_geometry && !m_lastLoadFailed)
		Load();

	W_SAFE_REMOVEREF(m_img);

	m_img = img;

	if (img) {
		SetSize(img->GetWidth(), img->GetHeight());
		img->AddReference();
	}

	m_material->SetTexture(1, m_img);
}

void WSprite::SetMaterial(WMaterial* mat) {
	W_SAFE_REMOVEREF(m_material);

	if (mat) {
		m_material = mat;
		mat->AddReference();
	}
}

void WSprite::SetPosition(float x, float y) {
	m_pos = WVector2(x, y);
	m_geometry_changed = true;
}

void WSprite::SetPosition(WVector2 pos) {
	m_pos = pos;
	m_geometry_changed = true;
}

void WSprite::Move(float units) {
	m_pos += WVec2TransformNormal(WVector2(1.0f, 0.0f), WRotationMatrixZ(m_angle)) * units;
	m_geometry_changed = true;
}

void WSprite::SetSize(float sizeX, float sizeY) {
	m_size = WVector2(sizeX, sizeY);
	m_geometry_changed = true;
}

void WSprite::SetSize(WVector2 size) {
	m_size = size;
	m_geometry_changed = true;
}

void WSprite::SetAngle(float fAngle) {
	m_angle = W_DEGTORAD(fAngle);
	m_geometry_changed = true;
}

void WSprite::Rotate(float fAngle) {
	m_angle += W_DEGTORAD(fAngle);
	m_geometry_changed = true;
}

void WSprite::SetRotationCenter(float x, float y) {
	m_rotationCenter = WVector2(x, y);
	m_geometry_changed = true;
}

void WSprite::SetRotationCenter(WVector2 center) {
	m_rotationCenter = center;
	m_geometry_changed = true;
}

void WSprite::SetPriority(unsigned int priority) {
	m_priority = priority;
	m_geometry_changed = true;
}

void WSprite::SetAlpha(float fAlpha) {
	if (!m_geometry && !m_lastLoadFailed)
		Load();

	m_alpha = fAlpha;
	m_material->SetVariableFloat("alpha", m_alpha);
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
			if (m_geometry_changed) {
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
				m_geometry_changed = false;
			}

			geometry->UnmapVertexBuffer();
		}

		m_material->Bind(rt);
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

float WSprite::GetPositionX() const {
	return m_pos.x;
}

float WSprite::GetPositionY() const {
	return m_pos.y;
}

WVector2 WSprite::GetPosition() const {
	return m_pos;
}

float WSprite::GetSizeX() const {
	return m_size.x;
}

float WSprite::GetSizeY() const {
	return m_size.y;
}

WVector2 WSprite::GetSize() const {
	return m_size;
}

unsigned int WSprite::GetPriority() const {
	return m_priority;
}


bool WSprite::Valid() const {
	return (m_material && m_material->Valid() && m_geometry && m_geometry->Valid()) && m_size.x > 0.0f && m_size.y >= 0.0f;
}
