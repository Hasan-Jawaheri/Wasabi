#pragma once

#include "Wasabi.h"

class WForwardRenderer : public WRenderer {
	VkSampler					m_sampler;

	class WEffect*				m_default_fx;

public:
	WForwardRenderer(Wasabi* const app);

	virtual WError				Initiailize();
	virtual WError				Render(class WRenderTarget* rt);
	virtual void				Cleanup();
	virtual WError				Resize(unsigned int width, unsigned int height);

	virtual class WMaterial*	CreateDefaultMaterial();
	virtual VkSampler			GetDefaultSampler() const;
};

class WFRMaterial : public WMaterial {
public:
	WFRMaterial(Wasabi* const app, unsigned int ID = 0);

	WError Texture(class WImage* img);
	WError SetColor(WColor col);
};
