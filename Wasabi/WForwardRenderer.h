#pragma once

#include "Wasabi.h"

class WForwardRenderer : public WRenderer {
	friend class WFRMaterial; // TODO: can be removed
	VkSampler					m_sampler;
	struct LightStruct*			m_lights;
	int							m_numLights;
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

	virtual WError Bind(class WRenderTarget* rt);

	WError Texture(class WImage* img);
	WError SetColor(WColor col);
};
