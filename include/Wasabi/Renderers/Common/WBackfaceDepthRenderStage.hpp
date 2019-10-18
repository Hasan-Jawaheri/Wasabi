#pragma once

#include "Wasabi/Renderers/WRenderStage.hpp"
#include "Wasabi/Renderers/Common/WRenderFragment.hpp"
#include "Wasabi/Materials/WEffect.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Objects/WObject.hpp"

class WBackfaceDepthRenderStageObjectPS : public WShader {
public:
	WBackfaceDepthRenderStageObjectPS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
};

/*
 * Implementation of a forward rendering stage that renders objects and terrains with simple lighting.
 * Creating this stage adds the following engine parameters:
 * * "maxLights": Maximum number of lights that can be rendered at once (Default is (void*)16)
 */
class WBackfaceDepthRenderStage : public WRenderStage {
	WObjectsRenderFragment* m_objectsFragment;
	WObjectsRenderFragment* m_animatedObjectsFragment;
	class WMaterial* m_perFrameMaterial;
	class WMaterial* m_perFrameAnimatedMaterial;

public:
	WBackfaceDepthRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter);
	virtual void Cleanup();
	virtual WError Resize(uint32_t width, uint32_t height);
};
