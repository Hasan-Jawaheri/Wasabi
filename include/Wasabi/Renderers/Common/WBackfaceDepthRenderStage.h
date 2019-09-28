#pragma once

#include "Wasabi/Renderers/WRenderStage.h"
#include "Wasabi/Renderers/Common/WRenderFragment.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Objects/WObject.h"

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
	class WMaterial* m_perFrameMaterial;

public:
	WBackfaceDepthRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter);
	virtual void Cleanup();
	virtual WError Resize(uint32_t width, uint32_t height);
};
