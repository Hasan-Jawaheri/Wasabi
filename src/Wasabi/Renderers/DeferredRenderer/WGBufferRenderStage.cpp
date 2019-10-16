#include "Wasabi/Renderers/DeferredRenderer/WGBufferRenderStage.h"
#include "Wasabi/Core/WCore.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Objects/WObject.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Cameras/WCamera.h"

WGBufferVS::WGBufferVS(Wasabi* const app) : WShader(app) {}

void WGBufferVS::Load(bool bSaveData) {
	m_desc = GetDesc();
	vector<uint8_t> code {
		#include "Shaders/gbuffer.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

W_SHADER_DESC WGBufferVS::GetDesc() {
	W_SHADER_DESC desc;
	desc.type = W_VERTEX_SHADER;
	desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerObject", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "color"), // object color
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isInstanced"), // whether or not instancing is enabled
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isTextured"), // whether or not to use diffuse texture
		}),
		W_BOUND_RESOURCE(W_TYPE_UBO, 1, 1, "uboPerFrame", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 0, "instancingTexture"),
	};
	desc.input_layouts = { W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 1), // texture index
	})  };
	return desc;
}

WGBufferAnimatedVS::WGBufferAnimatedVS(Wasabi* const app) : WShader(app) {}

void WGBufferAnimatedVS::Load(bool bSaveData) {
	m_desc = GetDesc();
	vector<uint8_t> code{
		#include "Shaders/gbuffer-animated.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

W_SHADER_DESC WGBufferAnimatedVS::GetDesc() {
	W_SHADER_DESC desc;
	desc.type = W_VERTEX_SHADER;
	desc.bound_resources = {
		WGBufferVS::GetDesc().bound_resources[0],
		WGBufferVS::GetDesc().bound_resources[1],
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 0, "animationTexture"),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 0, "instancingTexture"),
	};
	desc.input_layouts = { 
		WGBufferVS::GetDesc().input_layouts[0], W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone index
		W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weight
	}) };
	return desc;
}

WGBufferPS::WGBufferPS(Wasabi* const app) : WShader(app) {}

void WGBufferPS::Load(bool bSaveData) {
	m_desc = WGBufferPS::GetDesc();
	vector<uint8_t> code {
		#include "Shaders/gbuffer.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

W_SHADER_DESC WGBufferPS::GetDesc() {
	W_SHADER_DESC desc;
	desc.type = W_FRAGMENT_SHADER;
	desc.bound_resources = {
		WGBufferVS::GetDesc().bound_resources[0],
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 0, "diffuseTexture", {}, 8),
	};
	return desc;
}

WGBufferRenderStage::WGBufferRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BUFFER;
	m_stageDescription.depthOutput = WRenderStage::OUTPUT_IMAGE("GBufferDepth", VK_FORMAT_D16_UNORM, WColor(1.0f, 0.0f, 0.0f, 0.0f));
	m_stageDescription.colorOutputs = std::vector<WRenderStage::OUTPUT_IMAGE>({
		WRenderStage::OUTPUT_IMAGE("GBufferDiffuse", VK_FORMAT_R8G8B8A8_UNORM, WColor(0.0f, 0.0f, 0.0f, 0.0f)),
		WRenderStage::OUTPUT_IMAGE("GBufferViewSpaceNormal", VK_FORMAT_R8G8B8A8_UNORM, WColor(0.0f, 0.0f, 0.0f, 0.0f)),
	});
	m_stageDescription.flags = RENDER_STAGE_FLAG_PICKING_RENDER_STAGE;

	m_objectsFragment = nullptr;
	m_animatedObjectsFragment = nullptr;
	m_perFrameMaterial = nullptr;
	m_perFrameAnimatedMaterial = nullptr;
}

WError WGBufferRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	WGBufferVS* vs = new WGBufferVS(m_app);
	vs->SetName("GBufferDefaultVS");
	m_app->FileManager->AddDefaultAsset(vs->GetName(), vs);
	vs->Load();

	WGBufferAnimatedVS* vsa = new WGBufferAnimatedVS(m_app);
	vsa->SetName("GBufferDefaultAnimatedVS");
	m_app->FileManager->AddDefaultAsset(vsa->GetName(), vsa);
	vsa->Load();

	WGBufferPS* ps = new WGBufferPS(m_app);
	ps->SetName("GBufferDefaultPS");
	m_app->FileManager->AddDefaultAsset(ps->GetName(), ps);
	ps->Load();

	WEffect* GBufferFX = new WEffect(m_app);
	GBufferFX->SetName("GBufferDefaultEffect");
	m_app->FileManager->AddDefaultAsset(GBufferFX->GetName(), GBufferFX);

	WEffect* GBufferAnimatedFX = new WEffect(m_app);
	GBufferAnimatedFX->SetName("GBufferDefaultAnimatedEffect");
	m_app->FileManager->AddDefaultAsset(GBufferAnimatedFX->GetName(), GBufferAnimatedFX);

	err = GBufferFX->BindShader(vs);
	if (err) {
		err = GBufferFX->BindShader(ps);
		if (err) {
			err = GBufferFX->BuildPipeline(m_renderTarget);
			if (err) {
				err = GBufferAnimatedFX->BindShader(vsa);
				if (err) {
					err = GBufferAnimatedFX->BindShader(ps);
					if (err) {
						err = GBufferAnimatedFX->BuildPipeline(m_renderTarget);
					}
				}
			}
		}
	}
	W_SAFE_REMOVEREF(ps);
	W_SAFE_REMOVEREF(vs);
	W_SAFE_REMOVEREF(vsa);
	if (!err) {
		W_SAFE_REMOVEREF(GBufferFX);
		W_SAFE_REMOVEREF(GBufferAnimatedFX);
		return err;
	}

	m_objectsFragment = new WObjectsRenderFragment(m_stageDescription.name, false, GBufferFX, m_app, EFFECT_RENDER_FLAG_RENDER_GBUFFER);
	m_animatedObjectsFragment = new WObjectsRenderFragment(m_stageDescription.name + "-animated", true, GBufferAnimatedFX, m_app, EFFECT_RENDER_FLAG_RENDER_GBUFFER);

	m_perFrameMaterial = m_objectsFragment->GetEffect()->CreateMaterial(1);
	if (!m_perFrameMaterial)
		return WError(W_ERRORUNK);
	m_perFrameMaterial->SetName("GBufferPerFrameMaterial");
	m_app->FileManager->AddDefaultAsset(m_perFrameMaterial->GetName(), m_perFrameMaterial);

	m_perFrameAnimatedMaterial = m_animatedObjectsFragment->GetEffect()->CreateMaterial(1);
	if (!m_perFrameAnimatedMaterial)
		return WError(W_ERRORUNK);
	m_perFrameAnimatedMaterial->SetName("GBufferPerFrameAnimatedMaterial");
	m_app->FileManager->AddDefaultAsset(m_perFrameAnimatedMaterial->GetName(), m_perFrameAnimatedMaterial);

	return WError(W_SUCCEEDED);
}

void WGBufferRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_perFrameMaterial);
	W_SAFE_REMOVEREF(m_perFrameAnimatedMaterial);
	W_SAFE_DELETE(m_objectsFragment);
	W_SAFE_DELETE(m_animatedObjectsFragment);
}

WError WGBufferRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint32_t filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
		WCamera* cam = rt->GetCamera();

		m_perFrameMaterial->SetVariable<WMatrix>("viewMatrix", cam->GetViewMatrix());
		m_perFrameMaterial->SetVariable<WMatrix>("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameMaterial->Bind(rt);

		m_objectsFragment->Render(renderer, rt);

		m_perFrameAnimatedMaterial->SetVariable<WMatrix>("viewMatrix", cam->GetViewMatrix());
		m_perFrameAnimatedMaterial->SetVariable<WMatrix>("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameAnimatedMaterial->Bind(rt);

		m_animatedObjectsFragment->Render(renderer, rt);
	}

	return WError(W_SUCCEEDED);
}

WError WGBufferRenderStage::Resize(uint32_t width, uint32_t height) {
	return WRenderStage::Resize(width, height);
}
