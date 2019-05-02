#pragma once

#include "../WRenderStage.h"
#include "../../Materials/WEffect.h"
#include "../../Materials/WMaterial.h"
#include "../../Objects/WObject.h"

/*
 * Implementation of a forward rendering stage. Forward rendering is the simplest
 * rendering technique in graphics. Forward rendering renders every object
 * (or terrain, particles, etc...) straight to the backbuffer in one pass.
 * Forward rendering is usually fast as it has a low overhead.
 * This is a base class and must be implemented. An implementation must initialize
 * the WRenderStage description in the constructor.
 */
class WForwardRenderStage : public WRenderStage {
protected:
	class WEffect* m_renderEffect;
	bool m_setEffectDefault;
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

	virtual class WEffect* LoadRenderEffect() = 0;

public:
	WForwardRenderStage(class Wasabi* const app, bool backbuffer = false);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};
