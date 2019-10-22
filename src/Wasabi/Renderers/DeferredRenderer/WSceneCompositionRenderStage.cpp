#include "Wasabi/Renderers/DeferredRenderer/WSceneCompositionRenderStage.hpp"
#include "Wasabi/Core/WCore.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Images/WRenderTarget.hpp"
#include "Wasabi/Sprites/WSprite.hpp"
#include "Wasabi/Materials/WEffect.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Cameras/WCamera.hpp"
#include "Wasabi/Images/WImage.hpp"
#include "Wasabi/WindowAndInput/WWindowAndInputComponent.hpp"

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
	m_fullscreenSprite->SetName("SceneCompositionFullscreenSprite");
	uint32_t windowWidth = m_app->WindowAndInputComponent->GetWindowWidth();
	uint32_t windowHeight = m_app->WindowAndInputComponent->GetWindowHeight();
	m_fullscreenSprite->SetSize(WVector2((float)windowWidth, (float)windowHeight));
	m_fullscreenSprite->Hide();

	SceneCompositionPS* pixelShader = new SceneCompositionPS(m_app);
	pixelShader->Load();

	m_effect = m_app->SpriteManager->CreateSpriteEffect(m_renderTarget, pixelShader);
	W_SAFE_REMOVEREF(pixelShader);
	if (!m_effect)
		return WError(W_OUTOFMEMORY);

	m_perFrameMaterial = m_effect->CreateMaterial(0, true);
	m_constantsMaterial = m_effect->CreateMaterial(1, true);

	if (!m_perFrameMaterial || !m_constantsMaterial)
		return WError(W_ERRORUNK);

	m_currentCameraFarPlane = m_renderTarget->GetCamera()->GetMaxRange();
	SetAmbientLight(WColor(0.3f, 0.3f, 0.3f));
	SetSSAOSampleRadius(0.1f);
	SetSSAOIntensity(1.0f);
	SetSSAODistanceScale(3.0f);
	SetSSAOAngleBias(0.15f);
	m_constantsMaterial->SetVariable<float>("camFarClip", m_currentCameraFarPlane);

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
		m_constantsMaterial->SetVariable<float>("camFarClip", m_currentCameraFarPlane);
	}

	m_perFrameMaterial->SetVariable<WMatrix>("projInv", WMatrixInverse(cam->GetProjectionMatrix()));

	m_effect->Bind(rt);
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

void WSceneCompositionRenderStage::SetAmbientLight(WColor color) {
	m_constantsMaterial->SetVariable<WColor>("ambient", color);
}

void WSceneCompositionRenderStage::SetSSAOSampleRadius(float value) {
	m_constantsMaterial->SetVariable<float>("SSAOSampleRadius", value);

}

void WSceneCompositionRenderStage::SetSSAOIntensity(float value) {
	m_constantsMaterial->SetVariable<float>("SSAOIntensity", value);
}

void WSceneCompositionRenderStage::SetSSAODistanceScale(float value) {
	m_constantsMaterial->SetVariable<float>("SSAODistanceScale", value);
}

void WSceneCompositionRenderStage::SetSSAOAngleBias(float value) {
	m_constantsMaterial->SetVariable<float>("SSAOAngleBias", value);
}
