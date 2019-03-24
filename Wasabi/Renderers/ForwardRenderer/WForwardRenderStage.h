#pragma once

#include "../WRenderStage.h"
#include "../Common/ResourceCollection.h"
#include "../../Materials/WEffect.h"

struct LightStruct {
	WVector4 color;
	WVector4 dir;
	WVector4 pos;
	int type;
	int pad[3];
};

class WForwardRenderStageObjectVS : public WShader {
public:
	WForwardRenderStageObjectVS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc(int maxLights);
};

class WForwardRenderStageObjectPS : public WShader {
public:
	WForwardRenderStageObjectPS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc(int maxLights);
};

class WForwardRenderStage : public WRenderStage {
	class WEffect* m_defaultFX;
	class WMaterial* m_perFrameMaterial;
	ResourceCollection<class WObject, class WMaterial> m_objectMaterials;

	LightStruct* m_lights;

public:
	WForwardRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);

	/**
	 * Creates a material for a default object effect.
	 * @return  Newly allocated and initialized material
	 */
	class WMaterial* CreateDefaultObjectMaterial() const;
};
