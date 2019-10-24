#pragma once

#include "Wasabi/Renderers/WRenderStage.hpp"
#include "Wasabi/Renderers/Common/WRenderFragment.hpp"
#include "Wasabi/Materials/WEffect.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Objects/WObject.hpp"

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

class WForwardRenderStageAnimatedObjectVS : public WShader {
public:
	WForwardRenderStageAnimatedObjectVS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc(int maxLights);
};

class WForwardRenderStageObjectPS : public WShader {
public:
	WForwardRenderStageObjectPS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc(int maxLights);
};

class WForwardRenderStageTerrainVS : public WShader {
public:
	WForwardRenderStageTerrainVS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc(int maxLights);
};

class WForwardRenderStageTerrainPS : public WShader {
public:
	WForwardRenderStageTerrainPS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc(int maxLights);
};

/*
 * Implementation of a forward rendering stage that renders objects and terrains with simple lighting.
 * Creating this stage adds the following engine parameters:
 * * "maxLights": Maximum number of lights that can be rendered at once (Default is (void*)16)
 */
class WForwardRenderStage : public WRenderStage {
	WObjectsRenderFragment* m_objectsFragment;
	class WMaterial* m_perFrameObjectsMaterial;
	WObjectsRenderFragment* m_animatedObjectsFragment;
	class WMaterial* m_perFrameAnimatedObjectsMaterial;

	WTerrainRenderFragment* m_terrainsFragment;
	class WMaterial* m_perFrameTerrainsMaterial;

	std::vector<LightStruct> m_lights;

protected:
	bool m_addDefaultEffects; // @TODO please fix this mess

public:
	WForwardRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter);
	virtual void Cleanup();
	virtual WError Resize(uint32_t width, uint32_t height);

	void SetAmbientLight(WColor color);
};
