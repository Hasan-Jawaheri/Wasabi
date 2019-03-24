#if 0

#include "WDeferredRenderer.h"
#include "../../Geometries/WGeometry.h"
#include "../../Core/WCore.h"
#include "../../Cameras/WCamera.h"
#include "../../Materials/WEffect.h"
#include "../../Materials/WMaterial.h"
#include "../../Lights/WLight.h"
#include "../../Images/WRenderTarget.h"
#include "../../Images/WImage.h"
#include "../../Objects/WObject.h"
#include "../../Sprites/WSprite.h"
#include "../../Texts/WText.h"
#include "../../Particles/WParticles.h"

#include "../../WindowAndInput/WWindowAndInputComponent.h"

#include <functional>
using std::function;

class DeferredRendererPSDepth : public WShader {
public:
	DeferredRendererPSDepth(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 0),
		};
		LoadCodeGLSL(
			#include "Shaders/pixel_shader_depth.glsl"
		, bSaveData);
	}
};

class SpotLightVS : public WShader {
public:
	SpotLightVS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0,{
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "wvp"), // world * view * projection
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightDir"), // light direction (L vector)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "lightSpec"), // light specular
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightColor"), // light color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "intensity"), // light intensity
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "position"), // light color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "range"), // light range
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "minCosAngle"), // used for spot light (angle of beam precomputed)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "spotRadius"), // radius of the circle at the end of the cone
			}),
		};
	}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = {W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		})};
		LoadCodeGLSL(
			#include "Shaders/spot_light_vs.glsl"
		, bSaveData);
	}
};

class SpotLightPS : public WShader {
public:
	SpotLightPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 2),
			SpotLightVS::GetBoundResources()[0],
			W_BOUND_RESOURCE(W_TYPE_UBO, 3, {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projInv"), // inverse of projection
			}),
		};
		LoadCodeGLSL(
			#include "Shaders/spot_light_ps.glsl"
		, bSaveData);
	}
};

class PointLightVS : public WShader {
public:
	PointLightVS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "wvp"), // world * view * projection
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightDir"), // light direction (L vector)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "lightSpec"), // light specular
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightColor"), // light color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "intensity"), // light intensity
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "position"), // light position
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "range"), // light range
			}),
		};
	}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		}) };
		LoadCodeGLSL(
			#include "Shaders/point_light_vs.glsl"
		, bSaveData);
	}
};

class PointLightPS : public WShader {
public:
	PointLightPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 2),
			PointLightVS::GetBoundResources()[0],
			W_BOUND_RESOURCE(W_TYPE_UBO, 3, {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projInv"), // inverse of projection
			}),
		};
		LoadCodeGLSL(
			#include "Shaders/point_light_ps.glsl"
		, bSaveData);
	}
};

class DirectionalLightPS : public WShader {
public:
	DirectionalLightPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 2),
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "wvp"), // world * view * projection
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightDir"), // light direction (L vector)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "lightSpec"), // light specular
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "lightColor"), // light color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "intensity"), // light intensity
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3, "position"), // light position
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "range"), // light range
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "minCosAngle"), // used for spot light (angle of beam precomputed)
			}),
			W_BOUND_RESOURCE(W_TYPE_UBO, 3, {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projInv"), // inverse of projection
			}),
		};
		LoadCodeGLSL(
			#include "Shaders/directional_light_ps.glsl"
		, bSaveData);
	}
};

class SceneCompositionPS : public WShader {
public:
	SceneCompositionPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projInv"), // inverse of projection matrix
			}),
			W_BOUND_RESOURCE(W_TYPE_UBO, 1, {
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "ambient"), // light ambient color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "SSAOSampleRadius"), // Radius to sample SSAO occluders
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "SSAOIntensity"), // Intensity of SSAO
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "SSAODistanceScale"), // SSAO distance scaling between occluder and occludees
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "SSAOAngleBias"), // SSAO angle "cutoff" (0-1)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "camFarClip"), // Camera's far clip range
			}),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 2),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 3),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 4),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 5),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 6),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 7),
		};
		LoadCodeGLSL(
			#include "Shaders/scene_composition_ps.glsl"
		, bSaveData);
	}
};

class OnlyPositionGeometry : public WGeometry {
	const W_VERTEX_DESCRIPTION m_desc = W_VERTEX_DESCRIPTION({W_ATTRIBUTE_POSITION});

public:
	OnlyPositionGeometry(Wasabi* const app, unsigned int ID = 0) : WGeometry(app, ID) {}

	virtual unsigned int GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(unsigned int layout_index = 0) const {
		return m_desc;
	}
	virtual size_t GetVertexDescriptionSize(unsigned int layout_index = 0) const {
		return m_desc.GetSize();
	}

	virtual WError CreateFromDefaultVerticesData(vector<WDefaultVertex>& default_vertices, vector<uint>& indices, bool bDynamic) {
		vector<WVector3> custom_verts(default_vertices.size());
		for (unsigned int i = 0; i < default_vertices.size(); i++)
			custom_verts[i] = default_vertices[i].pos;
		return CreateFromData(custom_verts.data(), custom_verts.size(), indices.data(), indices.size(), bDynamic);
	}
};

WDeferredRenderer::WDeferredRenderer(Wasabi* const app) : WRenderer(app), m_GBuffer(this), m_LightBuffer(this) {
	m_sampler = VK_NULL_HANDLE;
	m_defaultFX = nullptr;
	m_backfaceFX = nullptr;
	m_masterRenderSprite = nullptr;
	m_compositionMaterial = nullptr;
	m_SSAORandomTexture = nullptr;
	m_ambientColor = WColor(0.5f, 0.5f, 0.5f);
	m_app->engineParams.insert(std::pair<std::string, void*>("maxLights", (void*)INT_MAX));
}

WError WDeferredRenderer::Initiailize() {
	Cleanup();

	//
	// Create the texture sampler
	//
	VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	// Max level-of-detail should match mip level count
	sampler.maxLod = 1;
	// Enable anisotropic filtering
	sampler.maxAnisotropy = 8;
	sampler.anisotropyEnable = VK_TRUE;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VkResult err = vkCreateSampler(m_device, &sampler, nullptr, &m_sampler);
	if (err != VK_SUCCESS) {
		Cleanup();
		return WError(W_OUTOFMEMORY);
	}

	//
	// Create the GBuffer
	//
	WError werr = m_GBuffer.Initialize(m_width, m_height);
	if (!werr) {
		Cleanup();
		return werr;
	}

	//
	// Create the default effect for rendering
	//
	VkPipelineRasterizationStateCreateInfo backface_rs = {};
	backface_rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	backface_rs.polygonMode = VK_POLYGON_MODE_FILL;
	backface_rs.cullMode = VK_CULL_MODE_FRONT_BIT;
	backface_rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
	backface_rs.depthClampEnable = VK_FALSE;
	backface_rs.rasterizerDiscardEnable = VK_FALSE;
	backface_rs.depthBiasEnable = VK_FALSE;
	backface_rs.lineWidth = 1.0f;

	WShader* vs = new DeferredRendererVS(m_app);
	vs->Load();
	WShader* ps = new DeferredRendererPS(m_app);
	ps->Load();
	WShader* psDepth = new DeferredRendererPSDepth(m_app);
	psDepth->Load();
	m_defaultFX = new WEffect(m_app);
	m_defaultFX->BindShader(vs);
	m_defaultFX->BindShader(ps);
	werr = m_defaultFX->BuildPipeline(m_GBuffer.m_renderTarget);
	if (werr) {
		m_backfaceFX = new WEffect(m_app);
		m_backfaceFX->BindShader(vs);
		m_backfaceFX->BindShader(psDepth);
		m_backfaceFX->SetRasterizationState(backface_rs);
		werr = m_backfaceFX->BuildPipeline(m_GBuffer.m_backfaceRenderTarget);
	}
	vs->RemoveReference();
	ps->RemoveReference();
	psDepth->RemoveReference();
	if (!werr)
		Cleanup();

	return werr;
}

WError WDeferredRenderer::LoadDependantResources() {
	VkPipelineColorBlendAttachmentState bs = {};
	VkPipelineDepthStencilStateCreateInfo dss = {};
	WShader* compositionPS = nullptr;

	//
	// Create the LightBuffer
	//
	WError werr = m_LightBuffer.Initialize(m_width, m_height);
	if (!werr)
		goto error;

	//
	// Create a randomized bump texture
	//
	m_SSAORandomTexture = new WImage(m_app);
	WVector2 pixels[64 * 64];
	for (int y = 0; y < 64; y++) {
		for (int x = 0; x < 64; x++) {
			pixels[y * 64 + x] = WVec2Normalize(WVector2(sinf(W_PI * (float)x/64.0f)+0.5, cosf(W_PI * (float)y / 64.0f)+0.5)) / 2.0f + WVector2(0.5f, 0.5f);
		}
	}
	werr = m_SSAORandomTexture->CreateFromPixelsArray(pixels, 64, 64, false, 2, VK_FORMAT_R32G32_SFLOAT, 4);
	werr = m_SSAORandomTexture->Load("noise.png");
	if (!werr)
		goto error;

	//
	// Create the final composition material
	//

	compositionPS = new SceneCompositionPS(m_app);
	compositionPS->Load();

	bs.blendEnable = VK_FALSE;
	bs.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	dss.depthTestEnable = VK_FALSE;
	dss.depthWriteEnable = VK_FALSE;
	dss.depthBoundsTestEnable = VK_FALSE;
	dss.stencilTestEnable = VK_FALSE;
	dss.front = dss.back;

	WEffect* compositionFX = m_app->SpriteManager->CreateSpriteEffect(compositionPS, bs, dss);
	if (!compositionFX)
		goto error;

	m_compositionMaterial = new WMaterial(m_app);
	werr = m_compositionMaterial->SetEffect(compositionFX);
	W_SAFE_REMOVEREF(compositionFX);
	if (!werr)
		goto error;

	werr = m_compositionMaterial->SetTexture(2, m_GBuffer.m_colorTarget);
	if (!werr)
		goto error;

	werr = m_compositionMaterial->SetTexture(3, m_LightBuffer.m_outputTarget);
	if (!werr)
		goto error;

	werr = m_compositionMaterial->SetTexture(4, m_GBuffer.m_normalTarget);
	if (!werr)
		goto error;

	werr = m_compositionMaterial->SetTexture(5, m_GBuffer.m_depthTarget);
	if (!werr)
		goto error;

	werr = m_compositionMaterial->SetTexture(6, m_GBuffer.m_backfaceDepthTarget);
	if (!werr)
		goto error;

	werr = m_compositionMaterial->SetTexture(7, m_SSAORandomTexture);
	if (!werr)
		goto error;

	m_masterRenderSprite = new WSprite(m_app);
	m_masterRenderSprite->SetMaterial(m_compositionMaterial);
	m_masterRenderSprite->Load();
	m_masterRenderSprite->SetSize(m_width, m_height);
	m_masterRenderSprite->Hide();

	SetAmbientLight(m_ambientColor);

	goto success;

error:
	Cleanup();

success:
	W_SAFE_REMOVEREF(compositionPS);

	return werr;
}

WError WDeferredRenderer::Resize(uint width, uint height) {
	WError werr = WRenderer::Resize(width, height);
	if (!werr)
		return werr;

	werr = m_GBuffer.Resize(width, height);
	if (!werr)
		return werr;

	werr = m_LightBuffer.Resize(width, height);
	if (!werr)
		return werr;

	m_masterRenderSprite->SetSize(width, height);

	return werr;
}

void WDeferredRenderer::Render(WRenderTarget* rt, unsigned int filter) {
	WError werr;

	//
	// Render to the GBuffer then to the LightBuffer
	//
	if (filter & RENDER_FILTER_OBJECTS) {
		m_GBuffer.Render(rt->GetCamera());
		m_LightBuffer.Render(rt->GetCamera());
	}
	
	//
	// Compose the GBuffer & LightBuffer onto the final render target
	//
	werr = rt->Begin();
	if (!werr)
		return;

	m_compositionMaterial->SetVariableMatrix("projInv", WMatrixInverse(rt->GetCamera()->GetProjectionMatrix()));

	static float radius = 0.1f;
	static float intensity = 1.0f;
	static float scale = 3.0f;
	static float bias = 0.15f;
	if (m_app->WindowAndInputComponent->KeyDown('Q'))
		radius += 0.0008f;
	if (m_app->WindowAndInputComponent->KeyDown('A'))
		radius -= 0.0008f;
	if (m_app->WindowAndInputComponent->KeyDown('W'))
		intensity += 0.004f;
	if (m_app->WindowAndInputComponent->KeyDown('S'))
		intensity -= 0.004f;
	if (m_app->WindowAndInputComponent->KeyDown('E'))
		scale += 0.004f;
	if (m_app->WindowAndInputComponent->KeyDown('D'))
		scale -= 0.004f;
	if (m_app->WindowAndInputComponent->KeyDown('R'))
		bias += 0.002f;
	if (m_app->WindowAndInputComponent->KeyDown('F'))
		bias -= 0.002f;
	m_app->TextComponent->RenderText("radius: " + std::to_string(radius), 5, 40, 16);
	m_app->TextComponent->RenderText("intensity: " + std::to_string(intensity), 5, 60, 16);
	m_app->TextComponent->RenderText("scale: " + std::to_string(scale), 5, 80, 16);
	m_app->TextComponent->RenderText("bias: " + std::to_string(bias), 5, 100, 16);
	m_compositionMaterial->SetVariableFloat("SSAOSampleRadius", radius);
	m_compositionMaterial->SetVariableFloat("SSAOIntensity", intensity);
	m_compositionMaterial->SetVariableFloat("SSAODistanceScale", scale);
	m_compositionMaterial->SetVariableFloat("SSAOAngleBias", bias);
	m_compositionMaterial->SetVariableFloat("camFarClip", rt->GetCamera()->GetMaxRange());

	m_masterRenderSprite->Show();
	m_masterRenderSprite->Render(rt);
	m_masterRenderSprite->Hide();

	//
	// Render particles
	//
	m_app->ParticlesManager->Render(rt);

	//
	// Render 2D assets
	//
	if (filter & RENDER_FILTER_SPRITES) {
		m_app->SpriteManager->Render(rt);
	}

	if (filter & RENDER_FILTER_TEXT)
		m_app->TextComponent->Render(rt);

	rt->End(false);
}

void WDeferredRenderer::Cleanup() {
	if (m_sampler)
		vkDestroySampler(m_device, m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;

	W_SAFE_REMOVEREF(m_SSAORandomTexture);
	W_SAFE_REMOVEREF(m_defaultFX);
	W_SAFE_REMOVEREF(m_backfaceFX);
	W_SAFE_REMOVEREF(m_masterRenderSprite);
	W_SAFE_REMOVEREF(m_compositionMaterial);
	m_GBuffer.Cleanup();
	m_LightBuffer.Cleanup();
}

WMaterial* WDeferredRenderer::CreateDefaultMaterial() {
	WMaterial* mat = new WMaterial(m_app);
	mat->SetEffect(m_defaultFX);
	return mat;
}

VkSampler WDeferredRenderer::GetTextureSampler(W_TEXTURE_SAMPLER_TYPE type) const {
	return m_sampler;
}

void WDeferredRenderer::SetAmbientLight(WColor col) {
	m_ambientColor = col;
	if (m_compositionMaterial)
		m_compositionMaterial->SetVariableColor("ambient", col);
}

WDeferredRenderer::GBufferStage::GBufferStage(WDeferredRenderer* renderer) : m_renderer(renderer) {
	m_renderTarget = nullptr;
	m_depthTarget = nullptr;
	m_backfaceDepthTarget = nullptr;
	m_colorTarget = nullptr;
	m_normalTarget = nullptr;
	m_backfaceRenderTarget = nullptr;
}

WDeferredRenderer::GBufferStage::~GBufferStage() {
	Cleanup();
}

void WDeferredRenderer::GBufferStage::Cleanup() {
	W_SAFE_REMOVEREF(m_renderTarget);
	W_SAFE_REMOVEREF(m_depthTarget);
	W_SAFE_REMOVEREF(m_colorTarget);
	W_SAFE_REMOVEREF(m_normalTarget);
	W_SAFE_REMOVEREF(m_backfaceDepthTarget);
	W_SAFE_REMOVEREF(m_backfaceRenderTarget);
}

WError WDeferredRenderer::GBufferStage::Initialize(unsigned int width, unsigned int height) {
	Cleanup();

	m_depthTarget = new WImage(m_renderer->m_app);
	m_backfaceDepthTarget = new WImage(m_renderer->m_app);
	m_colorTarget = new WImage(m_renderer->m_app);
	m_normalTarget = new WImage(m_renderer->m_app);
	m_renderTarget = new WRenderTarget(m_renderer->m_app);
	m_backfaceRenderTarget = new WRenderTarget(m_renderer->m_app);
	return Resize(m_renderer->m_width, m_renderer->m_height);
}

WError WDeferredRenderer::GBufferStage::Render(class WCamera* cam) {
	//
	// first pass: normal render
	//
	m_renderTarget->SetCamera(cam);
	WError werr = m_renderTarget->Begin();
	if (!werr)
		return werr;

	m_renderer->m_app->ObjectManager->Render(m_renderTarget);

	werr = m_renderTarget->End();
	if (!werr)
		return werr;

	//
	// second pass: render backfaces
	//
	m_backfaceRenderTarget->SetCamera(cam);
	werr = m_backfaceRenderTarget->Begin();
	if (!werr)
		return werr;

	uint numObjects = m_renderer->m_app->ObjectManager->GetEntitiesCount();
	for (uint i = 0; i < numObjects; i++)
		m_renderer->m_app->ObjectManager->GetEntityByIndex(i)->Render(m_backfaceRenderTarget, m_renderer->m_backfaceFX, false, false);

	werr = m_backfaceRenderTarget->End();
	if (!werr)
		return werr;

	return werr;
}

WError WDeferredRenderer::GBufferStage::Resize(unsigned int width, unsigned int height) {
	WError werr;

	if (m_renderTarget) {
		werr = m_depthTarget->CreateFromPixelsArray(nullptr, width, height, false, 1, VK_FORMAT_D16_UNORM, 2);
		if (!werr)
			return werr;

		werr = m_backfaceDepthTarget->CreateFromPixelsArray(nullptr, width, height, false, 1, VK_FORMAT_D16_UNORM, 2);
		if (!werr)
			return werr;

		werr = m_colorTarget->CreateFromPixelsArray(nullptr, width, height, false, 4, VK_FORMAT_R8G8B8A8_UNORM, 1);
		if (!werr)
			return werr;

		werr = m_normalTarget->CreateFromPixelsArray(nullptr, width, height, false, 4, VK_FORMAT_R8G8B8A8_UNORM, 1);
		if (!werr)
			return werr;

		werr = m_renderTarget->Create(width, height, vector<WImage*>({ m_colorTarget, m_normalTarget }), m_depthTarget);
		if (!werr)
			return werr;

		werr = m_backfaceRenderTarget->Create(width, height, vector<WImage*>(), m_backfaceDepthTarget);
		if (!werr)
			return werr;

		m_renderTarget->SetClearColor(WColor(0, 0, 0, 0), 0);
		m_renderTarget->SetClearColor(WColor(0, 0, 0, 0), 1);
		m_renderTarget->SetClearColor(WColor(1, 0, 0, 0), 2);

		
		m_backfaceRenderTarget->SetClearColor(WColor(1, 0, 0, 0), 0);
	}

	return werr;
}

void WDeferredRenderer::LightBufferStage::LightTypeAssets::Destroy() {
	for (auto it = material_map.begin(); it != material_map.end(); it++)
		W_SAFE_REMOVEREF(it->second);
	W_SAFE_REMOVEREF(geometry);
	W_SAFE_REMOVEREF(effect);
	W_SAFE_REMOVEREF(fullscreen_sprite);
	material_map.clear();
}

WDeferredRenderer::LightBufferStage::LightBufferStage(WDeferredRenderer* renderer) : m_renderer(renderer), m_blendState({}), m_rasterizationState({}) {
	m_renderTarget = nullptr;
	m_outputTarget = nullptr;
}

WDeferredRenderer::LightBufferStage::~LightBufferStage() {
	Cleanup();
}

void WDeferredRenderer::LightBufferStage::Cleanup() {
	m_renderer->m_app->LightManager->RemoveChangeCallback("renderer");
	for (auto iter = m_lightRenderingAssets.begin(); iter != m_lightRenderingAssets.end(); iter++)
		iter->second.Destroy();
	m_lightRenderingAssets.clear();
	W_SAFE_REMOVEREF(m_renderTarget);
	W_SAFE_REMOVEREF(m_outputTarget);
}

WError WDeferredRenderer::LightBufferStage::Initialize(unsigned int width, unsigned int height) {
	Cleanup();

	m_outputTarget = new WImage(m_renderer->m_app);
	m_renderTarget = new WRenderTarget(m_renderer->m_app);

	WError werr = Resize(m_renderer->m_width, m_renderer->m_height);
	if (!werr)
		return werr;

	m_blendState.colorWriteMask = 0xff;
	m_blendState.blendEnable = VK_TRUE;
	m_blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_blendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.colorBlendOp = VK_BLEND_OP_ADD;
	m_blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.alphaBlendOp = VK_BLEND_OP_ADD;

	m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	m_rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
	m_rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	m_rasterizationState.depthClampEnable = VK_FALSE;
	m_rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizationState.depthBiasEnable = VK_FALSE;
	m_rasterizationState.lineWidth = 1.0f;

	werr = _LoadDirectionalLightsAssets();
	if (!werr)
		return werr;

	werr = _LoadSpotLightsAssets();
	if (!werr)
		return werr;

	werr = _LoadPointLightsAssets();
	if (!werr)
		return werr;

	// Call OnLightsChange for all current lights and register a callback to catch all future changes
	for (unsigned int i = 0; i < m_renderer->m_app->LightManager->GetEntitiesCount(); i++)
		OnLightsChange(m_renderer->m_app->LightManager->GetEntityByIndex(i), true);
	m_renderer->m_app->LightManager->RegisterChangeCallback("renderer", [this](WLight* l, bool add) { this->OnLightsChange(l, add); });

	return werr;
}

void WDeferredRenderer::LightBufferStage::OnLightsChange(class WLight* light, bool is_added) {
	auto iter = m_lightRenderingAssets.find(light->GetType());
	if (iter == m_lightRenderingAssets.end())
		return;
	WDeferredRenderer::LightBufferStage::LightTypeAssets assets = iter->second;

	if (is_added) {
		WMaterial* material = new WMaterial(m_renderer->m_app);
		WError werr = material->SetEffect(assets.effect);
		if (werr == W_SUCCEEDED) {
			material->SetTexture(1, m_renderer->m_GBuffer.m_normalTarget);
			material->SetTexture(2, m_renderer->m_GBuffer.m_depthTarget);
		}
		iter->second.material_map.insert(std::pair<class WLight*, class WMaterial*>(light, material));
	}
	else {
		auto it = assets.material_map.find(light);
		if (it != assets.material_map.end()) {
			W_SAFE_REMOVEREF(it->second);
			iter->second.material_map.erase(light);
		}
	}
}

WError WDeferredRenderer::LightBufferStage::_LoadPointLightsAssets() {
	LightTypeAssets assets;
	WShader* vertex_shader = new PointLightVS(m_renderer->m_app);
	vertex_shader->Load();
	WShader* pixel_shader = new PointLightPS(m_renderer->m_app);
	pixel_shader->Load();
	assets.effect = new WEffect(m_renderer->m_app);
	WError werr = assets.effect->BindShader(vertex_shader);
	if (!werr)
		goto error;

	werr = assets.effect->BindShader(pixel_shader);
	if (!werr)
		goto error;

	assets.effect->SetBlendingState(m_blendState);
	assets.effect->SetRasterizationState(m_rasterizationState);
	
	werr = assets.effect->BuildPipeline(m_renderTarget);
	if (!werr)
		goto error;

	assets.geometry = new OnlyPositionGeometry(m_renderer->m_app);
	assets.geometry->CreateSphere(1.0f, 10, 10);

	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_POINT, assets));

error:
	W_SAFE_REMOVEREF(pixel_shader);
	W_SAFE_REMOVEREF(vertex_shader);

	return werr;
}

WError WDeferredRenderer::LightBufferStage::_LoadSpotLightsAssets() {
	LightTypeAssets assets;
	WShader* vertex_shader = new SpotLightVS(m_renderer->m_app);
	vertex_shader->Load();
	WShader* pixel_shader = new SpotLightPS(m_renderer->m_app);
	pixel_shader->Load();
	assets.effect = new WEffect(m_renderer->m_app);
	WError werr = assets.effect->BindShader(vertex_shader);
	if (!werr)
		goto error;

	werr = assets.effect->BindShader(pixel_shader);
	if (!werr)
		goto error;

	assets.effect->SetBlendingState(m_blendState);
	assets.effect->SetRasterizationState(m_rasterizationState);

	werr = assets.effect->BuildPipeline(m_renderTarget);
	if (!werr)
		goto error;

	assets.geometry = new OnlyPositionGeometry(m_renderer->m_app);
	assets.geometry->CreateCone(1.0f, 1.0f, 0, 16);
	assets.geometry->ApplyTransformation(WTranslationMatrix(0, -0.5, 0) * WRotationMatrixX(W_DEGTORAD(-90)));

	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_SPOT, assets));

error:
	W_SAFE_REMOVEREF(pixel_shader);
	W_SAFE_REMOVEREF(vertex_shader);

	return werr;
}

WError WDeferredRenderer::LightBufferStage::_LoadDirectionalLightsAssets() {
	LightTypeAssets assets;
	WError werr(W_SUCCEEDED);
	WShader* pixel_shader = new DirectionalLightPS(m_renderer->m_app);
	pixel_shader->Load();
	assets.effect = m_renderer->m_app->SpriteManager->CreateSpriteEffect(pixel_shader, m_blendState);
	if (!assets.effect) {
		werr = WError(W_OUTOFMEMORY);
		goto error;
	}

	assets.fullscreen_sprite = new WSprite(m_renderer->m_app);
	assets.fullscreen_sprite->Load(false);
	assets.fullscreen_sprite->SetSize(m_renderer->m_width, m_renderer->m_height);
	assets.fullscreen_sprite->Hide();

	m_lightRenderingAssets.insert(std::pair<int, LightTypeAssets>((int)W_LIGHT_DIRECTIONAL, assets));

error:
	W_SAFE_REMOVEREF(pixel_shader);

	return werr;
}

WError WDeferredRenderer::LightBufferStage::Render(class WCamera* cam) {
	m_renderTarget->SetCamera(cam);
	WError werr = m_renderTarget->Begin();
	if (!werr)
		return werr;

	unsigned int num_lights = m_renderer->m_app->LightManager->GetEntitiesCount();
	for (unsigned int i = 0; i < num_lights; i++) {
		WLight* light = m_renderer->m_app->LightManager->GetEntityByIndex(i);
		int light_type = (int)light->GetType();

		auto iter = m_lightRenderingAssets.find(light_type);
		if (iter == m_lightRenderingAssets.end())
			return WError(W_ERRORUNK);
		WDeferredRenderer::LightBufferStage::LightTypeAssets assets = iter->second;
		WMaterial* material = assets.material_map.find(light)->second;

		//
		// make sure the light is in the camera's frustum and is not hidden
		//
		if (light->Hidden())
			continue;
		if (light_type == W_LIGHT_SPOT) {
			if (!cam->CheckSphereInFrustum(light->GetPosition() + (light->GetLVector() * (light->GetRange() / 2.0f)), light->GetRange() / 2.0f))
				continue;
		}
		else if (light_type == W_LIGHT_POINT) {
			if (!cam->CheckSphereInFrustum(light->GetPosition(), light->GetRange()))
				continue;
		}

		//
		// setup the material for this light
		//
		WColor lightColor = light->GetColor();
		material->SetVariableMatrix("projInv", WMatrixInverse(cam->GetProjectionMatrix()));
		material->SetVariableVector3("lightDir", WVec3TransformNormal(light->GetLVector(), cam->GetViewMatrix()));
		material->SetVariableVector3("lightColor", WVector3(lightColor.r, lightColor.g, lightColor.b));
		material->SetVariableFloat("lightSpec", lightColor.a); // specular power is store in alpha component
		material->SetVariableFloat("intensity", light->GetIntensity());
		material->SetVariableMatrix("wvp", light->GetWorldMatrix() * cam->GetViewMatrix() * cam->GetProjectionMatrix());
		material->SetVariableFloat("range", light->GetRange());
		material->SetVariableVector3("position", WVec3TransformCoord(light->GetPosition(), cam->GetViewMatrix()));
		material->SetVariableFloat("minCosAngle", light->GetMinCosAngle());
		float emittingHalfAngle = acosf(light->GetMinCosAngle());
		float spotRadius = tanf(emittingHalfAngle) * light->GetRange();
		material->SetVariableFloat("spotRadius", spotRadius);

		//
		// render the light onto the light map
		//

		material->Bind(m_renderTarget);

		if (assets.fullscreen_sprite) {
			assets.fullscreen_sprite->Show();
			assets.fullscreen_sprite->Render(m_renderTarget);
			assets.fullscreen_sprite->Hide();
		}

		if (assets.geometry)
			assets.geometry->Draw(m_renderTarget);
	}

	werr = m_renderTarget->End();
	if (!werr)
		return werr;

	return werr;
}

WError WDeferredRenderer::LightBufferStage::Resize(unsigned int width, unsigned int height) {
	WError werr;

	if (m_renderTarget) {
		werr = m_outputTarget->CreateFromPixelsArray(nullptr, width, height, false, 4, VK_FORMAT_R8G8B8A8_UNORM, 1);
		if (!werr)
			return werr;

		werr = m_renderTarget->Create(width, height, m_outputTarget);
		if (!werr)
			return werr;
		m_renderTarget->SetClearColor(WColor(0, 0, 0, 0));
		
		for (auto iter = m_lightRenderingAssets.begin(); iter != m_lightRenderingAssets.end(); iter++)
			if (iter->second.fullscreen_sprite)
				iter->second.fullscreen_sprite->SetSize(width, height);
	}

	return werr;
}

#endif