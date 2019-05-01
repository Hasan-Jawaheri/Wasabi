#pragma once

#include "../WRenderStage.h"

class WSceneCompositionRenderStage : public WRenderStage {
	class WSprite* m_fullscreenSprite;
	class WEffect* m_effect;
	class WMaterial* m_perFrameMaterial;
	class WMaterial* m_constantsMaterial;
	float m_currentCameraFarPlane;

public:
	WSceneCompositionRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};
