#pragma once

#include "Wasabi/Renderers/WRenderStage.hpp"
#include "Wasabi/Renderers/ForwardRenderer/WForwardRenderStage.hpp"

class WSceneCompositionRenderStage : public WForwardRenderStage {
	class WSprite* m_fullscreenSprite;
	class WEffect* m_effect;
	class WMaterial* m_perFrameMaterial;
	class WMaterial* m_constantsMaterial;
	float m_currentCameraFarPlane;

public:
	WSceneCompositionRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter);
	virtual void Cleanup();
	virtual WError Resize(uint32_t width, uint32_t height);

	/**
	 * Composition parameters
	 */
	void SetAmbientLight(WColor color);
	void SetSSAOSampleRadius(float value);
	void SetSSAOIntensity(float value);
	void SetSSAODistanceScale(float value);
	void SetSSAOAngleBias(float value);
};
