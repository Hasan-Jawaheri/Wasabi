#if 0

#include "WGBufferRenderStage.h"
#include "../WRenderer.h"
#include "../../Objects/WObject.h"
#include "../../Images/WRenderTarget.h"
#include "../../Materials/WMaterial.h"

WGBufferVS::WGBufferVS(Wasabi* const app) : WShader(app) {
}

void WGBufferVS::Load(bool bSaveData) {
	m_desc.type = W_VERTEX_SHADER;
	m_desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_UBO, 1, "uboPerObject", {
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
			W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "animationTextureWidth"), // width of the animation texture
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "instanceTextureWidth"), // width of the instance texture
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isAnimated"), // whether or not animation is enabled
			W_SHADER_VARIABLE_INFO(W_TYPE_INT, "isInstanced"), // whether or not instancing is enabled
		}),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, "animationTexture"),
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, "instancingTexture"),
	};
	m_desc.animation_texture_index = 2;
	m_desc.instancing_texture_index = 3;
	m_desc.input_layouts = { W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
		W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
	}), W_INPUT_LAYOUT({
		W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone index
		W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weight
	}) };
	LoadCodeGLSL(
		#include "Shaders/vertex_shader.glsl"
	, bSaveData);
}

WGBufferPS::WGBufferPS(Wasabi* const app) : WShader(app) {
}

void WGBufferPS::Load(bool bSaveData) {
	m_desc.type = W_FRAGMENT_SHADER;
	m_desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 0, "diffuseTexture"),
	};
	LoadCodeGLSL(
		#include "Shaders/pixel_shader.glsl"
	, bSaveData);
}

WGBufferRenderStage::WGBufferRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BUFFER;
	m_stageDescription.depthOutput = WRenderStage::OUTPUT_IMAGE("GBufferDepth", WColor(1.0f, 0.0f, 0.0f, 0.0f));
	m_stageDescription.colorOutputs = std::vector<WRenderStage::OUTPUT_IMAGE>({
		WRenderStage::OUTPUT_IMAGE("GBufferDiffuse", WColor(0.0f, 0.0f, 0.0f, 0.0f)),
		WRenderStage::OUTPUT_IMAGE("GBufferViewSpaceNormal", WColor(0.0f, 0.0f, 0.0f, 0.0f)),
	});

	m_GBufferFX = nullptr;
}

WError WGBufferRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WRenderStage::Initialize(previousStages, width, height);

	WGBufferVS* vs = new WGBufferVS(m_app);
	vs->Load();

	WGBufferPS* ps = new WGBufferPS(m_app);
	ps->Load();

	m_GBufferFX = new WEffect(m_app);
	WError err = m_GBufferFX->BindShader(vs);
	if (err) {
		err = m_GBufferFX->BindShader(ps);
		if (err) {
			err = m_GBufferFX->BuildPipeline(m_app->Renderer->GetRenderTarget(m_stageDescription.name));
		}
	}
	W_SAFE_REMOVEREF(ps);
	W_SAFE_REMOVEREF(vs);
	if (!err)
		return err;

	m_objectMaterials.Initialize(m_stageDescription.name, m_app->ObjectManager, [this](WObject*) { return this->CreateDefaultObjectMaterial(); });

	return WError(W_SUCCEEDED);
}

WError WGBufferRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
		uint numObjects = m_app->ObjectManager->GetEntitiesCount();

		// render non-animated objects
		bool isDefaultFXBound = false;
		for (uint i = 0; i < numObjects; i++) {
			WObject* object = m_app->ObjectManager->GetEntityByIndex(i);
			if (!object->GetAnimation()) {
				WMaterial* material = m_objectMaterials.GetResourceOf(object);
				if (material) {
					m_GBufferFX->Bind(rt, { material }, 1, !isDefaultFXBound);
					isDefaultFXBound = true;
					object->Render(rt);
				}
			}
		}

		// render animated objects
		isDefaultFXBound = false;
		for (uint i = 0; i < numObjects; i++) {
			WObject* object = m_app->ObjectManager->GetEntityByIndex(i);
			if (object->GetAnimation()) {
				WMaterial* material = m_objectMaterials.GetResourceOf(object);
				if (material) {
					m_GBufferFX->Bind(rt, { material }, 2, !isDefaultFXBound);
					isDefaultFXBound = true;
					object->Render(rt);
				}
			}
		}
	}
}

void WGBufferRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_GBufferFX);
	m_objectMaterials.Cleanup();
}

WError WGBufferRenderStage::Resize(uint width, uint height) {
	WRenderStage::Resize(width, height);
}

WMaterial* WGBufferRenderStage::CreateDefaultObjectMaterial() const {
	WMaterial* mat = new WMaterial(m_app);
	if (!mat->CreateForEffect(m_GBufferFX))
		W_SAFE_REMOVEREF(mat);
	return mat;
}

#endif