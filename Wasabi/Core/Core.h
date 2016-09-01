#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#else
#endif
#include "vulkan/vulkan.h"
#pragma comment (lib, "vulkan-1.lib")
#include "VkTools/vulkanswapchain.hpp"
#include "VkTools/vulkantools.h"

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <array>
#include <chrono>

#include "WError.h"
#include "WManager.h"
#include "WBase.h"
#include "WOrientation.h"
#include "WTimer.h"
#include "WMath.h"
#include "WUtilities.h"

using namespace std;
using std::ios;
using std::basic_filebuf;
using std::vector;
using std::array;

#define W_SAFE_RELEASE(x) { if ( x ) { x->Release ( ); x = NULL; } }
#define W_SAFE_REMOVEREF(x) { if ( x ) { x->RemoveReference ( ); x = NULL; } }
#define W_SAFE_DELETE(x) { if ( x ) { delete x; x = NULL; } }
#define W_SAFE_DELETE_ARRAY(x) { if ( x ) { delete[] x; x = NULL; } }
#define W_SAFE_ALLOC(x) GlobalAlloc ( GPTR, x )
#define W_SAFE_FREE(x) { if ( x ) { GlobalFree ( x ); } }

#ifndef max
#define max(a,b) (a>b?a:b)
#endif
#ifndef min
#define min(a,b) (a<b?a:b)
#endif

typedef unsigned int uint;

struct W_BUFFER {
	VkBuffer buf;
	VkDeviceMemory mem;

	W_BUFFER() : buf(VK_NULL_HANDLE), mem(VK_NULL_HANDLE) {}
	void Destroy(VkDevice device) {
		if (buf)
			vkDestroyBuffer(device, buf, nullptr);
		buf = VK_NULL_HANDLE;
	}
};

class Wasabi {
public:
	std::map<std::string, void*> engineParams;
	class WSoundComponent*		SoundComponent;
	class WWindowComponent*		WindowComponent;
	class WInputComponent*		InputComponent;
	class WTextComponent*		TextComponent;
	class WPhysicsComponent*	PhysicsComponent;
	class WRenderer*			Renderer;

	class WObjectManager*		ObjectManager;
	class WGeometryManager*		GeometryManager;
	class WEffectManager*		EffectManager;
	class WShaderManager*		ShaderManager;
	class WMaterialManager*		MaterialManager;
	class WCameraManager*		CameraManager;
	class WImageManager*		ImageManager;
	class WSpriteManager*		SpriteManager;
	class WRenderTargetManager*	RenderTargetManager;
	class WLightManager*		LightManager;
	class WAnimationManager*	AnimationManager;

	WTimer Timer;

	float FPS, maxFPS;
	class  WGameState* curState;
	bool __EXIT; // when set to true, the engine will exit asap

	Wasabi();
	~Wasabi();

	virtual WError		Setup() = 0;
	virtual bool		Loop(float fDeltaTime) = 0;
	virtual void		Cleanup() = 0;

	void				SwitchState(class WGameState* state);

	WError				StartEngine(int width, int height);
	virtual WError		Resize(unsigned int width, unsigned int height);

	VkInstance			GetVulkanInstance() const;
	VkPhysicalDevice	GetVulkanPhysicalDevice() const;
	VkDevice			GetVulkanDevice() const;
	VkQueue				GetVulkanGraphicsQeueue() const;
	VulkanSwapChain*	GetSwapChain();
	VkCommandPool		GetCommandPool() const;
	void				GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t * typeIndex) const;

	VkResult			BeginCommandBuffer();
	VkResult			EndCommandBuffer();
	VkCommandBuffer		GetCommandBuffer() const;

protected:
	virtual int			SelectGPU(std::vector<VkPhysicalDevice> devices);
	virtual void		SetupComponents();

private:
	VkInstance							m_vkInstance;
	VkPhysicalDevice					m_vkPhysDev;
	VkDevice							m_vkDevice;
	VkQueue								m_queue;
	VulkanSwapChain						m_swapChain;
	bool								m_swapChainInitialized;
	VkPhysicalDeviceProperties			m_deviceProperties;
	VkPhysicalDeviceFeatures			m_deviceFeatures;
	VkPhysicalDeviceMemoryProperties	m_deviceMemoryProperties;
	VkCommandPool						m_cmdPool;
	VkCommandBuffer						m_copyCommandBuffer;

	void								_DestroyResources();
};

class WGameState {
protected:
	Wasabi* const m_app;

public:
	WGameState(Wasabi* const a) : m_app(a) {}
	~WGameState(void) {}

	virtual void Load() {}
	virtual void Update(float fDeltaTime) {}
	virtual void OnKeydown(char c) {}
	virtual void OnKeyup(char c) {}
	virtual void OnInput(char c) {}
	virtual void Cleanup() {}
};

typedef unsigned int uint;

Wasabi* WInitialize();
