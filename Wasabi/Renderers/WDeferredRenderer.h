#pragma once

#include "../Core/Core.h"
#include "WRenderer.h"
#include "../Materials/WMaterial.h"

class WDeferredRenderer : public WRenderer {
	VkSampler					m_sampler;

	class WEffect*				m_default_fx;

public:
	WDeferredRenderer(Wasabi* const app);

	virtual WError				Initiailize();
	virtual void				Render(class WRenderTarget* rt, unsigned int filter = -1);
	virtual void				Cleanup();
	virtual WError				Resize(unsigned int width, unsigned int height);

	virtual class WMaterial*	CreateDefaultMaterial();
	virtual VkSampler			GetDefaultSampler() const;
};
