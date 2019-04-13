#pragma once

#include "../WRenderStage.h"
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
	class WEffect* m_GBufferFX;
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

public:
	WGBufferRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};

