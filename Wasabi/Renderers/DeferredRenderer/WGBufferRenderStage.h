#pragma once

#include "../WRenderStage.h"
#include "../Common/ResourceCollection.h"
#include "../../Materials/WEffect.h"

class WGBufferVS : public WShader {
public:
	WGBufferVS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
};

class WGBufferPS : public WShader {
public:
	WGBufferPS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
};

class WGBufferRenderStage : public WRenderStage {
	class WEffect* m_GBufferFX;

	ResourceCollection<class WObject, class WMaterial> m_objectMaterials;

public:
	WGBufferRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter) = 0;
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);

	/**
	 * Creates a material for a default object effect.
	 * @return  Newly allocated and initialized material
	 */
	class WMaterial* CreateDefaultObjectMaterial() const;
};

