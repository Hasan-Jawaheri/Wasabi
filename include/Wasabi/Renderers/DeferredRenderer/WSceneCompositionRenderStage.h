#pragma once

#include "Wasabi/Renderers/WRenderStage.h"

class WSceneCompositionRenderStage : public WRenderStage {
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
};
