#include "WSprite.h"

struct SpriteVertex {
	WVector2 pos, uv;
};

class SpriteGeometry : public WGeometry {
public:
	SpriteGeometry(Wasabi* const app) : WGeometry(app) {}

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
	m_spriteGeometry = nullptr;
	m_spriteMaterial = nullptr;
}

WSpriteManager::~WSpriteManager() {
	W_SAFE_REMOVEREF(m_spriteGeometry);
	W_SAFE_REMOVEREF(m_spriteMaterial);
}

WError WSpriteManager::Load() {
	WShader* vs = new SpriteVS(m_app);
	vs->Load();
	WShader* ps = new SpritePS(m_app);
	ps->Load();
	WEffect* textFX = new WEffect(m_app);
	WError err = textFX->BindShader(vs);
	if (!err) {
		vs->RemoveReference();
		ps->RemoveReference();
		textFX->RemoveReference();
		return err;
	}
	err = textFX->BindShader(ps);
	if (!err) {
		vs->RemoveReference();
		ps->RemoveReference();
		textFX->RemoveReference();
		return err;
	}

	VkPipelineColorBlendAttachmentState bs;
	bs.colorWriteMask = 0xf;
	bs.blendEnable = VK_TRUE;
	bs.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	bs.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	bs.colorBlendOp = VK_BLEND_OP_ADD;
	bs.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	bs.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	bs.alphaBlendOp = VK_BLEND_OP_ADD;
	bs.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	textFX->SetBlendingState(bs);

	VkPipelineDepthStencilStateCreateInfo dss = {};
	dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	dss.depthTestEnable = VK_FALSE;
	dss.depthWriteEnable = VK_FALSE;
	dss.depthBoundsTestEnable = VK_FALSE;
	dss.stencilTestEnable = VK_FALSE;
	dss.front = dss.back;
	textFX->SetDepthStencilState(dss);

	err = textFX->BuildPipeline(m_app->Renderer->GetDefaultRenderTarget());
	vs->RemoveReference();
	ps->RemoveReference();

	if (!err) {
		textFX->RemoveReference();
		return err;
	}

	m_spriteMaterial = new WMaterial(m_app);
	err = m_spriteMaterial->SetEffect(textFX);
	textFX->RemoveReference();

	if (!err) {
		W_SAFE_REMOVEREF(m_spriteMaterial);
		return err;
	}

	unsigned int num_verts = 4;
	unsigned int num_indices = 6;
	SpriteVertex* vb = new SpriteVertex[num_verts];
	DWORD* ib = new DWORD[num_indices];

	// tri 1
	ib[0] = 0;
	ib[1] = 1;
	ib[2] = 2;
	// tri 2
	ib[3] = 1;
	ib[4] = 3;
	ib[5] = 2;

	m_spriteGeometry = new SpriteGeometry(m_app);
	err = m_spriteGeometry->CreateFromData(vb, num_verts, ib, num_indices, true);
	delete[] vb;
	delete[] ib;

	if (!err) {
		W_SAFE_REMOVEREF(m_spriteMaterial);
		W_SAFE_REMOVEREF(m_spriteGeometry);
		return err;
	}

	return WError(W_SUCCEEDED);
}

void WSpriteManager::Render(WRenderTarget* rt) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			m_entities[i][j]->Render(rt);
		}
	}
}

std::string WSprite::GetTypeName() const {
	return "Sprite";
}

WSprite::WSprite(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_img = nullptr;
	m_angle = 0.0f;
	m_pos = WVector2();
	m_size = WVector2();
	m_rotationCenter = WVector2();
	m_priority = 0;
	m_alpha = 1.0f;

	app->SpriteManager->AddEntity(this);
}

WSprite::~WSprite() {
	if (m_img)
		m_img->RemoveReference();

	m_app->SpriteManager->RemoveEntity(this);
}

void WSprite::SetImage(WImage* img) {
	if (m_img)
		m_img->RemoveReference();

	m_img = img;

	if (img) {
		SetSize(img->GetWidth(), img->GetHeight());
		img->AddReference();
	}
}

void WSprite::SetPosition(float x, float y) {
	m_pos = WVector2(x, y);
}

void WSprite::SetPosition(WVector2 pos) {
	m_pos = pos;
}

void WSprite::Move(float units) {
	m_pos += WVec2TransformNormal(WVector2(1.0f, 0.0f), WRotationMatrixZ(m_angle)) * units;
}

void WSprite::SetSize(float sizeX, float sizeY) {
	m_size = WVector2(sizeX, sizeY);
}

void WSprite::SetSize(WVector2 size) {
	m_size = size;
}

void WSprite::SetAngle(float fAngle) {
	m_angle = W_DEGTORAD(fAngle);
}

void WSprite::Rotate(float fAngle) {
	m_angle += W_DEGTORAD(fAngle);
}

void WSprite::SetRotationCenter(float x, float y) {
	m_rotationCenter = WVector2(x, y);
}


void WSprite::SetRotationCenter(WVector2 center) {
	m_rotationCenter = center;
}

void WSprite::SetPriority(unsigned int priority) {
	m_priority = priority;
}

void WSprite::SetAlpha(float fAlpha) {
	m_alpha = fAlpha;
}

void WSprite::Render(WRenderTarget* rt) {
	float scrWidth = m_app->WindowComponent->GetWindowWidth();
	float scrHeight = m_app->WindowComponent->GetWindowHeight();
	if (!m_hidden && Valid()) {
		SpriteVertex* vb;
		m_app->SpriteManager->m_spriteGeometry->MapVertexBuffer((void**)&vb);

		WVector2 rc = m_rotationCenter + m_pos;

		vb[0].pos = m_pos - rc;
		vb[0].uv = WVector2(0, 0);
		vb[1].pos = m_pos + WVector2(m_size.x, 0) - rc;
		vb[1].uv = WVector2(1, 0);
		vb[2].pos = m_pos + WVector2(0, m_size.y) - rc;
		vb[2].uv = WVector2(0, 1);
		vb[3].pos = m_pos + WVector2(m_size.x, m_size.y) - rc;
		vb[3].uv = WVector2(1, 1);

		WMatrix rotMtx = WRotationMatrixZ(m_angle) * WTranslationMatrix(rc.x, rc.y, 0);
		for (int i = 0; i < 4; i++) {
			WVector3 pos = WVector3(vb[i].pos.x, vb[i].pos.y, 0.0f);
			pos = WVec3TransformCoord(pos, rotMtx);
			vb[i].pos = WVector2(pos.x, pos.y);
			vb[i].pos = vb[i].pos * 2.0f / WVector2(scrWidth, scrHeight) - WVector2(1, 1);
		}

		m_app->SpriteManager->m_spriteGeometry->UnmapVertexBuffer();
		m_app->SpriteManager->m_spriteMaterial->SetVariableFloat("alpha", m_alpha);
		m_app->SpriteManager->m_spriteMaterial->SetTexture(1, m_img);
		m_app->SpriteManager->m_spriteMaterial->Bind(rt);
		m_app->SpriteManager->m_spriteGeometry->Draw(rt);
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
	return m_img && m_img->Valid() && m_size.x > 0.0f && m_size.y >= 0.0f;
}
