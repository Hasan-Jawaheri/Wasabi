/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine main header file
*********************************************************************/
#pragma once

#include "vulkan/vulkan.h"
#pragma comment (lib, "vulkan-1.lib")
#include "vulkanswapchain.hpp"
#include "vulkantools.h"

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <array>

#include "WError.h"
#include "WManager.h"
#include "WBase.h"

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

class Wasabi {
public:
	std::map<std::string, void*> engineParams;
	class WSoundComponent* SoundComponent;
	class WWindowComponent* WindowComponent;
	class WInputComponent* InputComponent;
	class WRenderer* Renderer;

	class WObjectManager* ObjectManager;
	class WGeometryManager* GeometryManager;

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
	WError				Resize(int width, int height);

	VkInstance			GetVulkanInstance() const;
	VkPhysicalDevice	GetVulkanPhysicalDevice() const;
	VkDevice			GetVulkanDevice() const;
	VkQueue				GetVulkanGraphicsQeueue() const;
	void				GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t * typeIndex) const;

protected:
	virtual int			SelectGPU(std::vector<VkPhysicalDevice> devices);
	virtual void		SetupComponents();

private:
	VkInstance							m_vkInstance;
	VkPhysicalDevice					m_vkPhysDev;
	VkDevice							m_vkDevice;
	VkQueue								m_queue;
	VkPhysicalDeviceProperties			m_deviceProperties;
	VkPhysicalDeviceFeatures			m_deviceFeatures;
	VkPhysicalDeviceMemoryProperties	m_deviceMemoryProperties;
};

class WGameState {
protected:
	Wasabi* const app;

public:
	WGameState(Wasabi* const a) : app(a) {}
	~WGameState(void) {}

	virtual void Load(void) {}
	virtual void Update(float fDeltaTime) {}
	virtual void OnKeydown(char c) {}
	virtual void OnKeyup(char c) {}
	virtual void OnInput(char c) {}
	virtual void Cleanup(void) {}
};

#include "WWindowComponent.h"
#include "WInputComponent.h"
#include "WRenderer.h"
#include "WForwardRenderer.h"
#include "WObjects.h"
#include "WGeometry.h"
#ifdef _WIN32
#include "WWC_Win32.h"
#include "WIC_Win32.h"
#elif defined(__linux__)
#include "WWC_Linux.h"
#include "WIC_Linux.h"
#endif

Wasabi* WInitialize();
