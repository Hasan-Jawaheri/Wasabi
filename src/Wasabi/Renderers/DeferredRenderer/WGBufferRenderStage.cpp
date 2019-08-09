#include "Renderers/DeferredRenderer/WGBufferRenderStage.h"
#include "Core/WCore.h"
#include "Renderers/WRenderer.h"
#include "Images/WRenderTarget.h"
#include "Objects/WObject.h"
#include "Materials/WMaterial.h"
#include "Cameras/WCamera.h"

WGBufferVS::WGBufferVS(Wasabi* const app) : WShader(app) {}

void WGBufferVS::Load(bool bSaveData) {
	m_desc = WGBufferVS::GetDesc();
	vector<byte> code = {
		#include "Shaders/gbuffer.vert.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
}

W_SHADER_DESC WGBufferVS::GetDesc() {
	W_SHADER_DESC desc;
	desc.type = W_VERTEX_SHADER;
	desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerObject", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "animationTextureWidth"), // width of the animation texture
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "instanceTextureWidth"), // width of the instance texture
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isAnimated"), // whether or not animation is enabled
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isInstanced"), // whether or not instancing is enabled
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "color"), // object color
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isTextured"), // whether or not to use diffuse texture
		}),
		W_BOUND_RESOURCE(W_TYPE_UBO, 1, 1, "uboPerFrame", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 0, "animationTexture"),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 0, "instancingTexture"),
	};
	desc.input_layouts = { W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 1), // texture index
	}), W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone index
		W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weight
	}) };
	return desc;
}

WGBufferPS::WGBufferPS(Wasabi* const app) : WShader(app) {}

void WGBufferPS::Load(bool bSaveData) {
	m_desc = WGBufferPS::GetDesc();
	vector<byte> code = {
		#include "Shaders/gbuffer.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
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
	m_perFrameMaterial = nullptr;
}

WError WGBufferRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	WGBufferVS* vs = new WGBufferVS(m_app);
	vs->SetName("GBufferDefaultVS");
	m_app->FileManager->AddDefaultAsset(vs->GetName(), vs);
	vs->Load();

	WGBufferPS* ps = new WGBufferPS(m_app);
	ps->SetName("GBufferDefaultPS");
	m_app->FileManager->AddDefaultAsset(ps->GetName(), ps);
	ps->Load();

	WEffect* GBufferFX = new WEffect(m_app);
	GBufferFX->SetName("GBufferDefaultEffect");
	m_app->FileManager->AddDefaultAsset(GBufferFX->GetName(), GBufferFX);
	err = GBufferFX->BindShader(vs);
	if (err) {
		err = GBufferFX->BindShader(ps);
		if (err) {
			err = GBufferFX->BuildPipeline(m_renderTarget);
		}
	}
	W_SAFE_REMOVEREF(ps);
	W_SAFE_REMOVEREF(vs);
	if (!err)
		return err;

	m_objectsFragment = new WObjectsRenderFragment(m_stageDescription.name, GBufferFX, m_app, true);

	m_perFrameMaterial = GBufferFX->CreateMaterial(1);
	if (!m_perFrameMaterial)
		return WError(W_ERRORUNK);
	m_perFrameMaterial->SetName("GBufferPerFrameMaterial");
	m_app->FileManager->AddDefaultAsset(m_perFrameMaterial->GetName(), m_perFrameMaterial);

	return WError(W_SUCCEEDED);
}

void WGBufferRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_perFrameMaterial);
	W_SAFE_DELETE(m_objectsFragment);
}

WError WGBufferRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
		WCamera* cam = rt->GetCamera();

		m_perFrameMaterial->SetVariableMatrix("viewMatrix", cam->GetViewMatrix());
		m_perFrameMaterial->SetVariableMatrix("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameMaterial->Bind(rt);

		m_objectsFragment->Render(renderer, rt);
	}

	return WError(W_SUCCEEDED);
}

WError WGBufferRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}
