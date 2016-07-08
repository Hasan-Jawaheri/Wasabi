#include "WSprite.h"

struct SpriteVertex {
	WVector2 pos, uv;
};

class SpriteGeometry : public WGeometry {
public:
	SpriteGeometry(Wasabi* const app) : WGeometry(app) {}

	virtual W_VERTEX_DESCRIPTION GetVertexDescription() const {
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
		m_desc.input_layout = W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // UV
		});
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
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4, "color"),
			}),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
		};
		LoadCodeGLSL(
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(binding = 0) uniform UBO {\n"
			"	vec4 color;\n"
			"} ubo;\n"
			"layout(binding = 1) uniform sampler2D sampler;\n"
			"layout(location = 0) in vec2 inUV;\n"
			"layout(location = 0) out vec4 outFragColor;\n"
			""
			"void main() {\n"
			"	float c = texture(sampler, inUV).r;\n"
			"	outFragColor = vec4(c) * ubo.color;\n"
			"}\n"
		);
	}
};

std::string WSpriteManager::GetTypeName() const {
	return "Sprite";
}

WSpriteManager::WSpriteManager(class Wasabi* const app) : WManager<WSprite>(app) {
}
WSpriteManager::~WSpriteManager() {
	m_spriteGeometry->RemoveReference();
	m_spriteMaterial->RemoveReference();
}

void WSpriteManager::Load() {
	WShader* vs = new SpriteVS(m_app);
	vs->Load();
	WShader* ps = new SpritePS(m_app);
	ps->Load();
	WEffect* textFX = new WEffect(m_app);
	textFX->BindShader(vs);
	textFX->BindShader(ps);

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

	textFX->BuildPipeline();
	vs->RemoveReference();
	ps->RemoveReference();
	m_spriteMaterial = new WMaterial(m_app);
	m_spriteMaterial->SetEffect(textFX);
	textFX->RemoveReference();

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
	m_spriteGeometry->CreateFromData(vb, num_verts, ib, num_indices, true);
	delete[] vb;
	delete[] ib;
}

void WSpriteManager::Render() {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			m_entities[i][j]->Render();
		}
	}
}

std::string WSprite::GetTypeName() const {
	return "Sprite";
}

WSprite::WSprite(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;

	app->SpriteManager->AddEntity(this);
}

WSprite::~WSprite() {
	m_app->SpriteManager->RemoveEntity(this);
}

void WSprite::Render() {
	if (!m_hidden) {
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

bool WSprite::Valid() const {
	return false; // TODO: do this
}
