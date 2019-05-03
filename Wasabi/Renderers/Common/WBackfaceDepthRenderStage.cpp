#include "WBackfaceDepthRenderStage.h"
#include "../ForwardRenderer/WForwardRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Lights/WLight.h"
#include "../../Images/WRenderTarget.h"
#include "../../Objects/WObject.h"
#include "../../Materials/WMaterial.h"
#include "../../Cameras/WCamera.h"

WBackfaceDepthRenderStageObjectPS::WBackfaceDepthRenderStageObjectPS(Wasabi* const app) : WShader(app) {}

void WBackfaceDepthRenderStageObjectPS::Load(bool bSaveData) {
	m_desc.type = W_FRAGMENT_SHADER;
	m_desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 0, "diffuseTexture", {}, 8),
	};
	vector<byte> code = {
		#include "Shaders/depth.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
}

WBackfaceDepthRenderStage::WBackfaceDepthRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BUFFER;
	m_stageDescription.depthOutput = WRenderStage::OUTPUT_IMAGE("BackfaceDepth", VK_FORMAT_D16_UNORM, WColor(1.0f, 0.0f, 0.0f, 0.0f));

	m_perFrameMaterial = nullptr;
	m_objectsFragment = nullptr;
}

WError WBackfaceDepthRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	WForwardRenderStageObjectVS* vs = new WForwardRenderStageObjectVS(m_app);
	vs->SetName("DefaultBackfaceVS");
	m_app->FileManager->AddDefaultAsset(vs->GetName(), vs);
	vs->Load();

	WBackfaceDepthRenderStageObjectPS* ps = new WBackfaceDepthRenderStageObjectPS(m_app);
	ps->SetName("DefaultBackfacePS");
	m_app->FileManager->AddDefaultAsset(ps->GetName(), ps);
	ps->Load();

	WEffect* fx = new WEffect(m_app);
	fx->SetName("DefaultBackfaceEffect");
	m_app->FileManager->AddDefaultAsset(fx->GetName(), fx);
	err = fx->BindShader(vs);
	if (err) {
		err = fx->BindShader(ps);
		if (err) {
			VkPipelineRasterizationStateCreateInfo rs = {};
			rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rs.polygonMode = VK_POLYGON_MODE_FILL;
			rs.cullMode = VK_CULL_MODE_FRONT_BIT;
			rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rs.depthClampEnable = VK_FALSE;
			rs.rasterizerDiscardEnable = VK_FALSE;
			rs.depthBiasEnable = VK_FALSE;
			rs.lineWidth = 1.0f;
			fx->SetRasterizationState(rs);
			err = fx->BuildPipeline(m_renderTarget);
		}
	}
	W_SAFE_REMOVEREF(vs);
	W_SAFE_REMOVEREF(ps);
	if (!err)
		return err;

	m_objectsFragment = new WObjectsRenderFragment(m_stageDescription.name, fx, m_app);

	m_perFrameMaterial = m_objectsFragment->GetEffect()->CreateMaterial(1);
	if (!m_perFrameMaterial) {
		err = WError(W_ERRORUNK);
	} else {
		m_perFrameMaterial->SetName("PerFrameForwardMaterial");
		m_app->FileManager->AddDefaultAsset(m_perFrameMaterial->GetName(), m_perFrameMaterial);
	}

	return err;
}

void WBackfaceDepthRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_perFrameMaterial);
	W_SAFE_DELETE(m_objectsFragment);
}

WError WBackfaceDepthRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
		WCamera* cam = rt->GetCamera();

		// create the per-frame UBO data
		m_perFrameMaterial->SetVariableMatrix("viewMatrix", cam->GetViewMatrix());
		m_perFrameMaterial->SetVariableMatrix("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameMaterial->SetVariableVector3("camPosW", cam->GetPosition());
		m_perFrameMaterial->Bind(rt);

		m_objectsFragment->Render(renderer, rt);
	}

	return WError(W_SUCCEEDED);
}

WError WBackfaceDepthRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}
