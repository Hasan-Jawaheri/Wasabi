#pragma once

#include "Wasabi/Renderers/WRenderStage.h"
#include "Wasabi/Renderers/Common/WRenderFragment.h"
#include "Wasabi/Materials/WEffect.h"

class WGBufferVS : public WShader {
public:
	WGBufferVS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc();
};

class WGBufferPS : public WShader {
public:
	WGBufferPS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc();
};

class WGBufferRenderStage : public WRenderStage {
	WObjectsRenderFragment* m_objectsFragment;
	class WMaterial* m_perFrameMaterial;

public:
	WGBufferRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter);
	virtual void Cleanup();
	virtual WError Resize(uint32_t width, uint32_t height);
};

