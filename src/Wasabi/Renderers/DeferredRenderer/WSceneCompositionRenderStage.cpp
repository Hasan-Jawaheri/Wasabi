#include "Wasabi/Renderers/DeferredRenderer/WSceneCompositionRenderStage.h"
#include "Wasabi/Core/WCore.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Sprites/WSprite.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Cameras/WCamera.h"
#include "Wasabi/Images/WImage.h"
#include "Wasabi/WindowAndInput/WWindowAndInputComponent.h"

class SceneCompositionPS : public WShader {
public:
	SceneCompositionPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, 0, "uboPerFrame", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projInv"), // inverse of projection matrix
			}),
			W_BOUND_RESOURCE(W_TYPE_UBO, 1, 1, "uboParams", {
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "ambient"), // light ambient color
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "SSAOSampleRadius"), // Radius to sample SSAO occluders
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "SSAOIntensity"), // Intensity of SSAO
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "SSAODistanceScale"), // SSAO distance scaling between occluder and occludees
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "SSAOAngleBias"), // SSAO angle "cutoff" (0-1)
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "camFarClip"), // Camera's far clip range
			}),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, 1, "diffuseTexture"),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, 1, "lightTexture"),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 4, 1, "normalTexture"),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 5, 1, "depthTexture"),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 6, 1, "backfaceDepthTexture"),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 7, 1, "randomTexture"),
		};
		vector<uint8_t> code {
			#include "Shaders/scene_composition.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

WSceneCompositionRenderStage::WSceneCompositionRenderStage(Wasabi* const app) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = RENDER_STAGE_TARGET_BACK_BUFFER;

	m_fullscreenSprite = nullptr;
	m_effect = nullptr;
	m_perFrameMaterial = nullptr;
	m_constantsMaterial = nullptr;
}

WError WSceneCompositionRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	m_fullscreenSprite = m_app->SpriteManager->CreateSprite();
	if (!m_fullscreenSprite)
		return WError(W_OUTOFMEMORY);
	uint32_t windowWidth = m_app->WindowAndInputComponent->GetWindowWidth();
	uint32_t windowHeight = m_app->WindowAndInputComponent->GetWindowHeight();
	m_fullscreenSprite->SetSize(WVector2((float)windowWidth, (float)windowHeight));

	VkPipelineColorBlendAttachmentState bs = {};
	bs.blendEnable = VK_FALSE;
	bs.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	SceneCompositionPS* pixelShader = new SceneCompositionPS(m_app);
	pixelShader->Load();

	m_effect = m_app->SpriteManager->CreateSpriteEffect(m_renderTarget, pixelShader, bs);
	W_SAFE_REMOVEREF(pixelShader);
	if (!m_effect)
		return WError(W_OUTOFMEMORY);

	m_perFrameMaterial = m_effect->CreateMaterial(0);
	m_constantsMaterial = m_effect->CreateMaterial(1);

	if (!m_perFrameMaterial || !m_constantsMaterial)
		return WError(W_ERRORUNK);

	m_currentCameraFarPlane = m_renderTarget->GetCamera()->GetMaxRange();
	m_constantsMaterial->SetVariableColor("ambient", WColor(0.3f, 0.3f, 0.3f));
	m_constantsMaterial->SetVariableFloat("SSAOSampleRadius", 0.1f);
	m_constantsMaterial->SetVariableFloat("SSAOIntensity", 1.0f);
	m_constantsMaterial->SetVariableFloat("SSAODistanceScale", 3.0f);
	m_constantsMaterial->SetVariableFloat("SSAOAngleBias", 0.15f);
	m_constantsMaterial->SetVariableFloat("camFarClip", m_currentCameraFarPlane);

	m_constantsMaterial->SetTexture("diffuseTexture", m_app->Renderer->GetRenderTargetImage("GBufferDiffuse"));
	m_constantsMaterial->SetTexture("lightTexture", m_app->Renderer->GetRenderTargetImage("LightBuffer"));
	m_constantsMaterial->SetTexture("normalTexture", m_app->Renderer->GetRenderTargetImage("GBufferViewSpaceNormal"));
	m_constantsMaterial->SetTexture("depthTexture", m_app->Renderer->GetRenderTargetImage("GBufferDepth"));
	WImage* backfaceDepthImg = m_app->Renderer->GetRenderTargetImage("BackfaceDepth");
	if (!backfaceDepthImg) {
		WColor pixels[1] = { WColor(0.0f, 0.0f, 0.0f, 0.0f) };
		backfaceDepthImg = m_app->ImageManager->CreateImage(pixels, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT);
		m_constantsMaterial->SetTexture("backfaceDepthTexture", backfaceDepthImg);
		backfaceDepthImg->RemoveReference();
	} else
		m_constantsMaterial->SetTexture("backfaceDepthTexture", backfaceDepthImg);
	//m_constantsMaterial->SetTexture("randomTexture", m_app->Renderer->GetRenderTargetImage(""));

	return WError(W_SUCCEEDED);
}

WError WSceneCompositionRenderStage::Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter) {
	UNREFERENCED_PARAMETER(renderer);
	UNREFERENCED_PARAMETER(filter);

	WCamera* cam = rt->GetCamera();
	if (abs(m_currentCameraFarPlane - cam->GetMaxRange()) >= W_EPSILON) {
		m_currentCameraFarPlane = cam->GetMaxRange();
		m_constantsMaterial->SetVariableFloat("camFarClip", m_currentCameraFarPlane);
	}

	m_effect->Bind(rt);
	m_constantsMaterial->Bind(rt);
	m_perFrameMaterial->SetVariableMatrix("projInv", WMatrixInverse(cam->GetProjectionMatrix()));
	m_perFrameMaterial->Bind(rt);
	m_fullscreenSprite->Render(rt);

	return WError(W_SUCCEEDED);
}

void WSceneCompositionRenderStage::Cleanup() {
	W_SAFE_REMOVEREF(m_fullscreenSprite);
	W_SAFE_REMOVEREF(m_effect);
	W_SAFE_REMOVEREF(m_perFrameMaterial);
	W_SAFE_REMOVEREF(m_constantsMaterial);
}

WError WSceneCompositionRenderStage::Resize(uint32_t width, uint32_t height) {
	if (m_fullscreenSprite)
		m_fullscreenSprite->SetSize(WVector2((float)width, (float)height));
	return WRenderStage::Resize(width, height);
}
