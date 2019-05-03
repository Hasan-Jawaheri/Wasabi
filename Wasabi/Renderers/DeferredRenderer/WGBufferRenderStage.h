#pragma once

#include "../WRenderStage.h"
#include "../Common/WRenderFragment.h"
#include "../../Materials/WEffect.h"

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

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};

