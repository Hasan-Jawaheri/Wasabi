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

class WForwardRenderStage : public WRenderStage {
	class WEffect* m_defaultFX;
	class WMaterial* m_perFrameMaterial;

	struct ObjectKey {
		class WEffect* fx;
		class WObject* obj;
		bool animated;

		ObjectKey(class WObject* object);
		const bool operator< (const ObjectKey& that) const;
	};
	/** Sorted container for all the objects to be rendered by this stage */
	std::map<ObjectKey, class WObject*> m_allObjects;

	LightStruct* m_lights;

public:
	WForwardRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};
