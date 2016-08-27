/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine renderer spec
*********************************************************************/
#pragma once

#include "Wasabi.h"

enum W_RENDER_FILTER {
	RENDER_FILTER_OBJECTS = 1,
	RENDER_FILTER_SPRITES = 2,
	RENDER_FILTER_TEXT = 4,
	RENDER_FILTER_PARTICLES = 8,
	RENDER_FILTER_TERRAIN = 16,
};

class WRenderer {
	friend int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow);
	friend class Wasabi;

	WError _Initialize();
	void _Render();
	void _Cleanup();

public:
	WRenderer(Wasabi* const app);

	virtual WError				Initiailize() = 0;
	virtual void				Render(class WRenderTarget* rt, unsigned int filter = -1) = 0;
	virtual void				Cleanup() = 0;
	virtual WError				Resize(unsigned int width, unsigned int height);

	virtual class WMaterial*	CreateDefaultMaterial() = 0;
	virtual VkSampler			GetDefaultSampler() const = 0;

	VkQueue						GetQueue() const;
	class WRenderTarget*		GetDefaultRenderTarget() const;

protected:
	Wasabi*						m_app;

	VkDevice					m_device;
	VkQueue						m_queue;
	VulkanSwapChain*			m_swapChain;
	WRenderTarget*				m_renderTarget;

	// Synchronization semaphores
	struct {
		VkSemaphore presentComplete; // Swap chain image presentation
		VkSemaphore renderComplete; // Command buffer submission and execution
	} m_semaphores;

	VkFormat					m_depthFormat, m_colorFormat;
	uint32_t					m_width, m_height;
	bool						m_resizing;
};

