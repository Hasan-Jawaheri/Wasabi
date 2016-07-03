/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine renderer spec
*********************************************************************/
#pragma once

#include "Wasabi.h"
#include "WMath.h"

class WRenderer {
public:
	WRenderer(Wasabi* app) : m_app(app) {}
	~WRenderer() {}

	virtual WError		Initiailize() = 0;
	virtual WError		Render() = 0;
	virtual void		Cleanup() = 0;

	virtual void		SetClearColor(WColor col) = 0;

	virtual class WMaterial*CreateDefaultMaterial() = 0;

	virtual VkQueue			GetQueue() const = 0;
	virtual VkRenderPass	GetRenderPass() const = 0;
	virtual VkPipelineCache	GetPipelineCache() const = 0;
	virtual VkCommandBuffer	GetCommnadBuffer() const = 0;
	virtual VkCommandPool	GetCommandPool() const = 0;

protected:
	Wasabi* m_app;
};

