#pragma once

#include "../Common/WForwardRenderStage.h"
#include "../../Materials/WEffect.h"
#include "../../Materials/WMaterial.h"
#include "../../Objects/WObject.h"

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

/*
 * Implementation of a forward rendering stage that renders objects and terrains with simple lighting.
 * Creating this stage adds the following engine parameters:
 * * "maxLights": Maximum number of lights that can be rendered at once (Default is (void*)16)
 */
class WForwardLightRenderStage : public WForwardRenderStage {
	class WMaterial* m_perFrameMaterial;
	std::vector<LightStruct> m_lights;

protected:
	virtual class WEffect* LoadRenderEffect();

public:
	WForwardLightRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};
