#pragma once

#include "Wasabi.h"

class WDeferredRenderer : public WRenderer {
	VkSampler					m_sampler;

	class WEffect*				m_default_fx;

public:
	WDeferredRenderer(Wasabi* const app);

	virtual WError				Initiailize();
	virtual WError				Render(class WRenderTarget* rt);
	virtual void				Cleanup();
	virtual WError				Resize(unsigned int width, unsigned int height);

	virtual class WMaterial*	CreateDefaultMaterial();
	virtual VkSampler			GetDefaultSampler() const;
};
