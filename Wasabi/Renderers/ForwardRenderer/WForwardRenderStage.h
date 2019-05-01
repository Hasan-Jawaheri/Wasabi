#pragma once

#include "../WRenderStage.h"
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
 * Implementation of a forward rendering stage. Forward rendering is the simplest
 * rendering technique in graphics. Forward rendering renders every object
 * (or terrain, particles, etc...) straight to the backbuffer in one pass.
 * Forward rendering is usually fast as it has a low overhead.
 * Creating this stage adds the following engine parameters:
 * * "maxLights": Maximum number of lights that can be rendered at once (Default is (void*)16)
 */
class WForwardRenderStage : public WRenderStage {
	class WEffect* m_defaultFX;
	class WMaterial* m_perFrameMaterial;
	uint m_currentMatId;

	struct ObjectKey {
		class WEffect* fx;
		class WObject* obj;
		bool animated;

		ObjectKey(class WObject* object);
		const bool operator< (const ObjectKey& that) const;
	};
	/** Sorted container for all the objects to be rendered by this stage */
	std::map<ObjectKey, class WObject*> m_allObjects;

	void OnObjectChange(class WObject* object, bool added);

	std::vector<LightStruct> m_lights;

public:
	WForwardRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};
