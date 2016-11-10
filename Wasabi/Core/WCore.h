/** @file WCore.h
 *  @brief Core of the engine.
 *
 *  This file contains the core of the engine (the Wasabi class) and various
 *  tools used by the engine.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

/** @defgroup engineclass Main Engine Classes
 */

#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#elif (defined __linux__)
#ifndef VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#endif
#endif
#include "vulkan/vulkan.h"
#pragma comment (lib, "vulkan-1.lib")
#include "VkTools/vulkanswapchain.hpp"
#include "VkTools/vulkantools.h"

#include <math.h>
#include <float.h>
#include <climits>
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

#define W_SAFE_RELEASE(x) { if ( x ) { (x)->Release ( ); x = NULL; } }
#define W_SAFE_REMOVEREF(x) { if ( x ) { (x)->RemoveReference ( ); x = NULL; } }
#define W_SAFE_DELETE(x) { if ( x ) { delete x; x = NULL; } }
#define W_SAFE_DELETE_ARRAY(x) { if ( x ) { delete[] x; x = NULL; } }
#define W_SAFE_ALLOC(x) malloc(x)
#define W_SAFE_FREE(x) { if ( x ) { free(x); } }

#ifndef _WIN32
#define fopen_s(a,b,c) *a = fopen(b, c)
#define strcpy_s(a,b,c) strcpy(a,c)
#define ZeroMemory(x,y) memset(x, 0, y)
#endif

typedef unsigned int uint;

/**
 * Wrapper for a Vulkan buffer an its backing memory.
 */
struct W_BUFFER {
	/** Vulkan buffer */
	VkBuffer buf;
	/** buf's backing memory */
	VkDeviceMemory mem;

	W_BUFFER() : buf(VK_NULL_HANDLE), mem(VK_NULL_HANDLE) {}

	/**
	 * Destroy the buffer and its backing memory
	 * @param device The Vulkan device used to crate the buffer
	 */
	void Destroy(VkDevice device) {
		if (buf)
			vkDestroyBuffer(device, buf, nullptr);
		if (mem)
			vkFreeMemory(device, mem, nullptr);
		buf = VK_NULL_HANDLE;
		mem = VK_NULL_HANDLE;
	}
};

/**
 * @ingroup engineclass
 * Main application class. This class is abstract and one must implement its
 * three methods (Setup, Loop and Cleanup) to be able to run the engine.
 */
class Wasabi {
public:
	/**
	 * A map of various parameters used by the engine. Built-in parameters are:
	 * * "appName": Pointer to the name of the application. Default is
	 * 	  (void*)"Wasabi".
	 * * "fontBmpSize": The size of the font bitmap when a new font is created.
	 * 		default is (void*)(512).
	 * * "fontBmpCharHeight": The height of each character when a new font bitmap
	 * 		is crated. Default is (void*)(32).
	 * * "fontBmpNumChars": Number of characters to put when a new font bitmap
	 * 		is created. Default is (void*)(96).
	 * * "textBatchSize": The maximum number of characters to be passed in for
	 * 		a single text draw. Default is (void*)(256).
	 * * "geometryImmutable": When set to true, created geometry will be
	 * 		immutable (more efficient and uses less memory, but loses all dynamic
	 * 		attributes). Default is (void*)(false).
	 * * "numGeneratedMips": Number of mipmaps to generate when a new image is
	 * 		crated. Default is (void*)(1).
	 */
	std::map<std::string, void*> engineParams;
	/** Pointer to the attached sound component */
	class WSoundComponent* SoundComponent;
	/** Pointer to the attached window component */
	class WWindowComponent* WindowComponent;
	/** Pointer to the attached input component */
	class WInputComponent* InputComponent;
	/** Pointer to the attached text component */
	class WTextComponent* TextComponent;
	/** Pointer to the attached physics component */
	class WPhysicsComponent* PhysicsComponent;
	/** Pointer to the attached renderer */
	class WRenderer* Renderer;

	/** Pointer to the object manager */
	class WObjectManager* ObjectManager;
	/** Pointer to the geometry manager */
	class WGeometryManager* GeometryManager;
	/** Pointer to the effect manager */
	class WEffectManager* EffectManager;
	/** Pointer to the shader manager */
	class WShaderManager* ShaderManager;
	/** Pointer to the material manager */
	class WMaterialManager* MaterialManager;
	/** Pointer to the camera manager */
	class WCameraManager* CameraManager;
	/** Pointer to the image manager */
	class WImageManager* ImageManager;
	/** Pointer to the sprite manager */
	class WSpriteManager* SpriteManager;
	/** Pointer to the render target manager */
	class WRenderTargetManager* RenderTargetManager;
	/** Pointer to the light manager */
	class WLightManager* LightManager;
	/** Pointer to the animation manager */
	class WAnimationManager* AnimationManager;

	/** A timer object, which starts counting when the application starts */
	WTimer Timer;

	/** Current FPS, set by the engine */
	float FPS;
	/** Maximum FPS the engine should reach, can be set by the user, 0 sets
	 *  no limit
	 */
	float maxFPS;
	/** Current game state */
	class  WGameState* curState;
	/** When set to true, the engine will exit asap */
	bool __EXIT;

	Wasabi();
	~Wasabi();

	/**
	 * This function must be implemented by an application. It is called after
	 * WInitialize and is supposed to call StartEngine() and initialize
	 * application resources.
	 * @return Any error returned other than W_SUCCEEDED will result in the end
	 *         of the program
	 */
	virtual WError Setup() = 0;

	/**
	 * This function must be implemented by an application. It is called by the
	 * engine every frame to allow the application to update its state.
	 * @param fDeltaTime  The step time for this frame (roughly 1 / FPS)
	 * @return            If this function returns true, execution continues,
	 *                    otherwise, the application will exit
	 */
	virtual bool Loop(float fDeltaTime) = 0;

	/**
	 * This function must be implemented by an application. It is called by the
	 * engine to give the application a last chance to clean up its resources
	 * before the engine exits.
	 */
	virtual void Cleanup() = 0;

	/**
	 * Switches the game state from the current state to the provided state.
	 * The previous state will have its Cleanup() method called, and the new
	 * state will have its Load() method called. The game states should be
	 * allocated by the user.
	 * @param state The new game state
	 */
	void SwitchState(class WGameState* state);

	/**
	 * This function must be called by the user before any Wasabi resources are
	 * initiated/created. This function must be called during the application's
	 * Setup(). This function will start the engine, initializing all its
	 * resources and components.
	 * @param  width  Width of the window when the engine starts
	 * @param  height Height of the window when the engine starts
	 * @return        Error code, see WError.h
	 */
	WError StartEngine(int width, int height);

	/**
	 * This function can be overloaded by the user. This function is called by
	 * the engine when the window size changed. An overloaded implementation
	 * must call Wasabi::Resize(width, height) to allow the engine to also
	 * perform its internal resizing procedure.
	 * @param  width  New window width
	 * @param  height New window height
	 * @return        Error code, see WError.h. Returned value's impact depends on
	 *                the currently used window component
	 */
	virtual WError Resize(unsigned int width, unsigned int height);

	/**
	 * Retrieves the Vulkan instance.
	 * @return The Vulkan instance
	 */
	VkInstance GetVulkanInstance() const;

	/**
	 * Retrieves the Vulkan physical device that the engine is using.
	 * @return The Vulkan physical device
	 */
	VkPhysicalDevice GetVulkanPhysicalDevice() const;

	/**
	 * Retrieves the virtual device that the engine is using.
	 * @return The Vulkan virtual device
	 */
	VkDevice GetVulkanDevice() const;

	/**
	 * Retrieves the currently used Vulkan graphics queue.
	 * @return The Vulkan graphics queue
	 */
	VkQueue GetVulkanGraphicsQeueue() const;

	/**
	 * Retrieves the currently used swap chain.
	 * @return The swap chain
	 */
	VulkanSwapChain* GetSwapChain();

	/**
	 * Retrieves an engine-initialized Vulkan command pool.
	 * @return A Vulkan command pool
	 */
	VkCommandPool GetCommandPool() const;

	/**
	 * Retrieves the index of a Vulakn memory type that is compatible with the
	 * requested memory type and properties
	 * @param typeBits   A 32-bit value, in which each bit represents a usable
	 *                   memory type
	 * @param properties The requested memory properties to be found
	 * @param typeIndex  Pointer to an index to be filled
	 */
	void GetMemoryType(uint typeBits, VkFlags properties,
					   uint* typeIndex) const;

	/**
	 * Starts recording commands on the dummy command buffer, which can be
	 * acquired using GetCommandBuffer().
	 * @return A Vulkan result, VK_SUCCESS on success
	 */
	VkResult BeginCommandBuffer();

	/**
	 * Ends recording commands on the dummy command buffer and submits it to the
	 * graphics queue.
	 * @return A Vulkan result, VK_SUCCESS on success
	 */
	VkResult EndCommandBuffer();

	/**
	 * Retrieves the dummy command buffer that is used with BeginCommandBuffer()
	 * and EndCommandBuffer().
	 * @return The dummy command buffer
	 */
	VkCommandBuffer GetCommandBuffer() const;

protected:
	/**
	 * This function can be overloaded by the application. This function gives
	 * the application a chance to select the physical device (A graphics card)
	 * from the list available to Vulkan. This function is called by the engine
	 * during StartEngine() and the selected physical device will be used.
	 * @param  devices The list of available physical devices
	 * @return         Must return an index into the list devices, which will
	 *                 be used by the engine
	 */
	virtual int SelectGPU(std::vector<VkPhysicalDevice> devices);

	/**
	 * This function can be overloaded by the application. This function is
	 * called by the engine in StartEngine() and will give the application a
	 * chance to set the components of the engine manually.
	 */
	virtual void SetupComponents();

private:
	/** The Vulkan instance */
	VkInstance m_vkInstance;
	/** The used Vulkan physical device */
	VkPhysicalDevice m_vkPhysDev;
	/** The used Vulkan virtual device */
	VkDevice m_vkDevice;
	/** The used graphics queue */
	VkQueue m_queue;
	/** The swap chain */
	VulkanSwapChain m_swapChain;
	/** true if the swap chain has been initialized yet, false otherwise */
	bool m_swapChainInitialized;
	/** Vulkan properties of the physical devices */
	VkPhysicalDeviceProperties m_deviceProperties;
	/** Vulkan features of the physical device */
	VkPhysicalDeviceFeatures m_deviceFeatures;
	/** Memory types available on the device */
	VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties;
	/** Command pool created by the engine */
	VkCommandPool m_cmdPool;
	/** A dummy command buffer for general use */
	VkCommandBuffer m_copyCommandBuffer;

	/**
	 * Destroys all resources of the engine.
	 */
	void _DestroyResources();
};

/**
 * This class represents a game state. Game states can be used to completely
 * separate different phases of a game. For example, one can crate a game
 * state for the pre-menu credits, a state for the game menu and a state for
 * the actual game.
 *
 * States need to be allocated and freed by the user.
 */
class WGameState {
protected:
	/** Pointer to the application (the Wasabi instance) */
	Wasabi* const m_app;

public:
	WGameState(Wasabi* const a) : m_app(a) {}
	~WGameState() {}

	/**
	 * This function, will be called every time this state is switched to. This
	 * gives the state a chance to load its resources.
	 */
	virtual void Load() {}

	/**
	 * This function is called every frame (right after Wasabi::Loop()) while
	 * the state is active.
	 * @param fDeltaTime The step time for this frame (roughly 1 / FPS)
	 */
	virtual void Update(float fDeltaTime) {}

	/**
	 * This function is called whenever a key is pushed down while this state is
	 * active.
	 * @param c The pushed key
	 */
	virtual void OnKeydown(char c) {}

	/**
	 * This function is called whenever a key is released while this state is
	 * active.
	 * @param c The released key
	 */
	virtual void OnKeyup(char c) {}

	/**
	 * This function is called whenever character input is received while this
	 * state is active. Character input is supplied by the OS, and is best
	 * suitable for capturing input for things like text boxes.
	 * @param c [description]
	 */
	virtual void OnInput(char c) {}

	/**
	 * This function is called before this state is switched from. This gives the
	 * state a chance to clean up its resources before its destroyed.
	 */
	virtual void Cleanup() {}
};

typedef unsigned int uint;

/**
 * This function is not defined by the library. It must be defined by the user.
 * This function will be called at the beginning of the program, and will
 * expect the user-defined code to create a newly allocated instance of an
 * application, which should be a child of a Wasabi class.
 *
 * @return A newly allocated Wasabi child that will run the engine
 */
Wasabi* WInitialize();
