#include "WSpritesRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Images/WRenderTarget.h"
#include "../../Sprites/WSprite.h"
#include "../../Materials/WMaterial.h"

WSpriteVS::WSpriteVS(Wasabi* const app) : WShader(app) {}
void WSpriteVS::Load(bool bSaveData) {
	m_desc.type = W_VERTEX_SHADER;
	m_desc.input_layouts = { W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
	}) };
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
	, bSaveData);
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
	LoadCodeGLSL(
		"#version 450\n"
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"#extension GL_ARB_shading_language_420pack : enable\n"
		""
		"layout(binding = 0) uniform UBO {\n"
		"	float alpha;\n"
		"} uboPerSprite;\n"
		"layout(binding = 1) uniform sampler2D textureDiffuse;\n"
		"layout(location = 0) in vec2 inUV;\n"
		"layout(location = 0) out vec4 outFragColor;\n"
		""
		"void main() {\n"
		"	vec4 c = texture(textureDiffuse, inUV);\n"
		"	outFragColor = vec4(c.rgb, c.a * uboPerSprite.alpha);\n"
		"}\n"
	, bSaveData);
}

WSpritesRenderStage::SpriteKey::SpriteKey(WSprite* spr) {
	sprite = spr;
	fx = spr->GetDefaultEffect();
	priority = spr->GetPriority();
}

const bool WSpritesRenderStage::SpriteKey::operator< (const SpriteKey& that) const {
	if (priority != that.priority)
		return priority < that.priority;

	return (void*)fx < (void*)that.fx ? true : fx == that.fx ? (sprite < that.sprite) : false;
}

WSpritesRenderStage::WSpritesRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = backbuffer ? RENDER_STAGE_TARGET_BACK_BUFFER : RENDER_STAGE_TARGET_PREVIOUS;

	m_spriteFX = nullptr;
}

WError WSpritesRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WRenderStage::Initialize(previousStages, width, height);

	m_spriteVertexShader = new WSpriteVS(m_app);
	m_spriteVertexShader->Load();

	m_spritePixelShader = new WSpritePS(m_app);
	m_spritePixelShader->Load();

	m_spriteFX = CreateSpriteEffect();
	if (!m_spriteFX)
		return WError(W_ERRORUNK);

	m_app->SpriteManager->RegisterChangeCallback(m_stageDescription.name, [this](WSprite* s, bool added) {
		if (added) {
			if (!s->GetDefaultEffect())
				s->AddEffect(this->m_spriteFX);
			SpriteKey key(s);
			this->m_allSprites.insert(std::make_pair(key, s));
		} else {
			auto iter = this->m_allSprites.find(SpriteKey(s));
			if (iter != this->m_allSprites.end())
				this->m_allSprites.erase(iter);
			else {
				// the sprite seems to have changed and then removed before we cloud reindex it in the render loop
				// need to find it manually now...
				for (auto it = this->m_allSprites.begin(); it != this->m_allSprites.end(); it++) {
					if (it->second == s) {
						this->m_allSprites.erase(it);
						break;
					}
				}
			}
		}
	});

	return WError(W_SUCCEEDED);
}

WError WSpritesRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_SPRITES) {
		WEffect* boundFX = nullptr;
		std::vector<SpriteKey> reindexSprites;
		for (auto it = m_allSprites.begin(); it != m_allSprites.end(); it++) {
			WSprite* sprite = it->second;
			WEffect* effect = sprite->GetDefaultEffect();
			WMaterial* material = sprite->GetMaterial(effect);
			if (material && sprite->WillRender(rt)) {
				uint priority = sprite->GetPriority();
				if (boundFX != effect) {
					effect->Bind(rt);
					boundFX = effect;
				}
				if (priority != it->first.priority || effect != it->first.fx)
					reindexSprites.push_back(it->first);

				material->Bind(rt);
				sprite->Render(rt);
			}
		}
		for (auto it = reindexSprites.begin(); it != reindexSprites.end(); it++) {
			m_allSprites.erase(*it);
			m_allSprites.insert(std::make_pair(SpriteKey(it->sprite), it->sprite));
		}
	}

	return WError(W_SUCCEEDED);
}

void WSpritesRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_spriteFX);
	m_app->SpriteManager->RemoveChangeCallback(m_stageDescription.name);
}

WError WSpritesRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}


WEffect* WSpritesRenderStage::CreateSpriteEffect(
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

	err = spriteFX->BuildPipeline(m_app->Renderer->GetRenderTarget(m_stageDescription.name));

	if (!err) {
		spriteFX->RemoveReference();
		return nullptr;
	}

	return spriteFX;
}

WShader* WSpritesRenderStage::GetSpriteVertexShader() const {
	return m_spriteVertexShader;
}

WShader* WSpritesRenderStage::GetSpritePixelShader() const {
	return m_spritePixelShader;
}

