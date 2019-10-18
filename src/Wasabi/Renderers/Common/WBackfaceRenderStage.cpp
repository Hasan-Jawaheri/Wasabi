#include "Wasabi/Renderers/Common/WBackfaceDepthRenderStage.hpp"
#include "Wasabi/Renderers/ForwardRenderer/WForwardRenderStage.hpp"
#include "Wasabi/Core/WCore.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Lights/WLight.hpp"
#include "Wasabi/Images/WRenderTarget.hpp"
#include "Wasabi/Objects/WObject.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Cameras/WCamera.hpp"

WBackfaceDepthRenderStageObjectPS::WBackfaceDepthRenderStageObjectPS(Wasabi* const app) : WShader(app) {}

void WBackfaceDepthRenderStageObjectPS::Load(bool bSaveData) {
	m_desc.type = W_FRAGMENT_SHADER;
	m_desc.bound_resources = {
		W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 0, "diffuseTexture", {}, 8),
	};
	vector<uint8_t> code {
		#include "Shaders/depth.frag.glsl.spv"
	};
	LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
}

WBackfaceDepthRenderStage::WBackfaceDepthRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BUFFER;
	m_stageDescription.depthOutput = WRenderStage::OUTPUT_IMAGE("BackfaceDepth", VK_FORMAT_D16_UNORM, WColor(1.0f, 0.0f, 0.0f, 0.0f));

	m_perFrameMaterial = nullptr;
	m_perFrameAnimatedMaterial = nullptr;
	m_objectsFragment = nullptr;
	m_animatedObjectsFragment = nullptr;
}

WError WBackfaceDepthRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	WForwardRenderStageObjectVS* vs = new WForwardRenderStageObjectVS(m_app);
	vs->SetName("DefaultBackfaceVS");
	m_app->FileManager->AddDefaultAsset(vs->GetName(), vs);
	vs->Load();

	WForwardRenderStageAnimatedObjectVS* vsa = new WForwardRenderStageAnimatedObjectVS(m_app);
	vsa->SetName("DefaultBackfaceAnimatedVS");
	m_app->FileManager->AddDefaultAsset(vsa->GetName(), vsa);
	vsa->Load();

	WBackfaceDepthRenderStageObjectPS* ps = new WBackfaceDepthRenderStageObjectPS(m_app);
	ps->SetName("DefaultBackfacePS");
	m_app->FileManager->AddDefaultAsset(ps->GetName(), ps);
	ps->Load();

	WEffect* fx = new WEffect(m_app);
	fx->SetName("DefaultBackfaceEffect");
	m_app->FileManager->AddDefaultAsset(fx->GetName(), fx);

	WEffect* fxa = new WEffect(m_app);
	fxa->SetName("DefaultBackfaceAnimatedEffect");
	m_app->FileManager->AddDefaultAsset(fxa->GetName(), fxa);

	VkPipelineRasterizationStateCreateInfo rs = {};
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_FRONT_BIT;
	rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;
	rs.lineWidth = 1.0f;

	err = fx->BindShader(vs);
	if (err) {
		err = fx->BindShader(ps);
		if (err) {
			fx->SetRasterizationState(rs);
			err = fx->BuildPipeline(m_renderTarget);
			if (err) {
				err = fxa->BindShader(vsa);
				if (err) {
					err = fxa->BindShader(ps);
					if (err) {
						fxa->SetRasterizationState(rs);
						err = fxa->BuildPipeline(m_renderTarget);
					}
				}
			}
		}
	}
	W_SAFE_REMOVEREF(vs);
	W_SAFE_REMOVEREF(vsa);
	W_SAFE_REMOVEREF(ps);
	if (!err) {
		W_SAFE_REMOVEREF(fx);
		W_SAFE_REMOVEREF(fxa);
		return err;
	}

	m_objectsFragment = new WObjectsRenderFragment(m_stageDescription.name, false, fx, m_app, EFFECT_RENDER_FLAG_RENDER_DEPTH_ONLY);
	m_animatedObjectsFragment = new WObjectsRenderFragment(m_stageDescription.name + "-animated", true, fxa, m_app, EFFECT_RENDER_FLAG_RENDER_DEPTH_ONLY);

	m_perFrameMaterial = m_objectsFragment->GetEffect()->CreateMaterial(1, true);
	if (!m_perFrameMaterial) {
		err = WError(W_ERRORUNK);
	} else {
		m_perFrameMaterial->SetName("PerFrameForwardMaterial");
		m_app->FileManager->AddDefaultAsset(m_perFrameMaterial->GetName(), m_perFrameMaterial);
	}

	m_perFrameAnimatedMaterial = m_animatedObjectsFragment->GetEffect()->CreateMaterial(1, true);
	if (!m_perFrameAnimatedMaterial) {
		err = WError(W_ERRORUNK);
	} else {
		m_perFrameAnimatedMaterial->SetName("PerFrameForwardAnimatedMaterial");
		m_app->FileManager->AddDefaultAsset(m_perFrameAnimatedMaterial->GetName(), m_perFrameAnimatedMaterial);
	}

	return err;
}

void WBackfaceDepthRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_REMOVEREF(m_perFrameMaterial);
	W_SAFE_REMOVEREF(m_perFrameAnimatedMaterial);
	W_SAFE_DELETE(m_objectsFragment);
	W_SAFE_DELETE(m_animatedObjectsFragment);
}

WError WBackfaceDepthRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint32_t filter) {
	if (filter & RENDER_FILTER_OBJECTS) {
		WCamera* cam = rt->GetCamera();

		// create the per-frame UBO data
		m_perFrameMaterial->SetVariable<WMatrix>("viewMatrix", cam->GetViewMatrix());
		m_perFrameMaterial->SetVariable<WMatrix>("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameMaterial->SetVariable<WVector3>("camPosW", cam->GetPosition());

		m_perFrameAnimatedMaterial->SetVariable<WMatrix>("viewMatrix", cam->GetViewMatrix());
		m_perFrameAnimatedMaterial->SetVariable<WMatrix>("projectionMatrix", cam->GetProjectionMatrix());
		m_perFrameAnimatedMaterial->SetVariable<WVector3>("camPosW", cam->GetPosition());

		m_objectsFragment->Render(renderer, rt);

		m_animatedObjectsFragment->Render(renderer, rt);
	}

	return WError(W_SUCCEEDED);
}

WError WBackfaceDepthRenderStage::Resize(uint32_t width, uint32_t height) {
	return WRenderStage::Resize(width, height);
}
