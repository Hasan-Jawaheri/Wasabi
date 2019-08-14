#include "Wasabi/Core/WCore.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Renderers/ForwardRenderer/WForwardRenderer.h"
#include "Wasabi/Renderers/DeferredRenderer/WDeferredRenderer.h"
#include "Wasabi/WindowAndInput/WWindowAndInputComponent.h"
#include "Wasabi/Objects/WObject.h"
#include "Wasabi/Geometries/WGeometry.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Cameras/WCamera.h"
#include "Wasabi/Images/WImage.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Sprites/WSprite.h"
#include "Wasabi/Lights/WLight.h"
#include "Wasabi/Animations/WAnimation.h"
#include "Wasabi/Physics/WPhysicsComponent.h"
#include "Wasabi/Sounds/WSound.h"
#include "Wasabi/Texts/WText.h"
#include "Wasabi/Particles/WParticles.h"
#include "Wasabi/Terrains/WTerrain.h"

#include "Wasabi/WindowAndInput/GLFW/WGLFWWindowAndInputComponent.h"

static std::vector<std::function<void()>> g_cleanupCalls;

#ifdef _WIN32
#include <Windows.h>
int main();
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow) {
	return main();
}
#endif

int main() {
	Wasabi* app = WInitialize();
	int ret = RunWasabi(app);
	W_SAFE_DELETE(app);
	for (auto it : g_cleanupCalls)
		it();
	return ret;
}

void WRegisterGlobalCleanup(std::function<void()> fun) {
	g_cleanupCalls.push_back(fun);
}

int RunWasabi(Wasabi* app) {
	if (app) {
		app->Timer.Start();
		if (app->Setup()) {
			unsigned int numFrames = 0;
			auto fpsTimer = std::chrono::high_resolution_clock::now();
			float maxFPSReached = app->maxFPS > 0.001f ? app->maxFPS : 60.0f;
			float deltaTime = 1.0f / maxFPSReached;
			app->FPS = 0;
			while (!app->__EXIT) {
				auto tStart = std::chrono::high_resolution_clock::now();
				app->Timer.GetElapsedTime(true); // record elapsed time

				if (app->WindowAndInputComponent && !app->WindowAndInputComponent->Loop())
					continue;

				if (deltaTime >= 0.00001f) {
					if (!app->Loop(deltaTime))
						break;
					if (app->curState)
						app->curState->Update(deltaTime);
					if (app->PhysicsComponent)
						app->PhysicsComponent->Step(deltaTime);
				}

				if (app->AnimationManager)
					app->AnimationManager->Update(deltaTime);
				if (app->Renderer)
					app->Renderer->Render();

				numFrames++;

				auto tEnd = std::chrono::high_resolution_clock::now();
				auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
				deltaTime = (float)tDiff / 1000.0f;
				maxFPSReached = fmax(maxFPSReached, 1.0f / deltaTime);

				// update FPS
				if (std::chrono::duration<double, std::milli>(tEnd - fpsTimer).count() / 1000.0f > 0.5f) {
					app->FPS = (float)numFrames / 0.5f;
					fpsTimer = std::chrono::high_resolution_clock::now();
					numFrames = 0;
				}

				if (app->maxFPS > 0.001) {
					float maxDeltaTime = 1.0f / app->maxFPS; // delta time at max FPS
					if (deltaTime < maxDeltaTime) {
						auto sleepStart = std::chrono::high_resolution_clock::now();
						double diff;
						do {
							auto sleepNow = std::chrono::high_resolution_clock::now();
							diff = std::chrono::duration<double, std::milli>(sleepNow - sleepStart).count();
						} while ((float)diff / 1000.0f < (maxDeltaTime - deltaTime));
						deltaTime = maxDeltaTime;
					}
					deltaTime = fmax(deltaTime, 1.0f / app->maxFPS); // dont let deltaTime be 0
				} else
					deltaTime = fmax(deltaTime, 1.0f / maxFPSReached); // dont let deltaTime be 0
			}

			app->Cleanup();
		}
	}

	return 0;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(
	VkDebugReportFlagsEXT       flags,
	VkDebugReportObjectTypeEXT  objectType,
	uint64_t                    object,
	size_t                      location,
	int32_t                     messageCode,
	const char*                 pLayerPrefix,
	const char*                 pMessage,
	void*                       pUserData) {
	((Wasabi*)pUserData)->WindowAndInputComponent->ShowErrorMessage(std::string(pMessage), !(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT));
	return VK_FALSE;
}

Wasabi::Wasabi() : Timer(W_TIMER_SECONDS, true) {
	engineParams = {
		{ "appName", (void*)"Wasabi" }, // LPCSTR
		{ "fontBmpSize", (void*)(512) }, // int
		{ "fontBmpCharHeight", (void*)(32) }, // int
		{ "fontBmpNumChars", (void*)(96) }, // int
		{ "textBatchSize", (void*)(256) }, // int
		{ "geometryImmutable", (void*)(false) }, // bool
		{ "numGeneratedMips", (void*)(1) }, // int
		{ "bufferingCount", (void*)(2) }, // int
		{ "enableVulkanValidation", (void*)(true) }, // bool
	};
	m_swapChainInitialized = false;
	
	MemoryManager = nullptr;
	SoundComponent = nullptr;
	WindowAndInputComponent = nullptr;
	TextComponent = nullptr;
	PhysicsComponent = nullptr;
	Renderer = nullptr;

	FileManager = nullptr;
	ObjectManager = nullptr;
	GeometryManager = nullptr;
	EffectManager = nullptr;
	ShaderManager = nullptr;
	MaterialManager = nullptr;
	CameraManager = nullptr;
	ImageManager = nullptr;
	SpriteManager = nullptr;
	RenderTargetManager = nullptr;
	LightManager = nullptr;
	AnimationManager = nullptr;
	ParticlesManager = nullptr;
	TerrainManager = nullptr;

	m_vkDevice = VK_NULL_HANDLE;
	m_vkInstance = VK_NULL_HANDLE;

	curState = nullptr;
	__EXIT = false;
	maxFPS = 60.0f;
}
Wasabi::~Wasabi() {
	_DestroyResources();
}

void Wasabi::_DestroyResources() {
	if (m_vkDevice)
		vkDeviceWaitIdle(m_vkDevice);

	if (WindowAndInputComponent)
		WindowAndInputComponent->Cleanup();
	if (Renderer)
		Renderer->Cleanup();
	if (PhysicsComponent)
		PhysicsComponent->Cleanup();

	W_SAFE_DELETE(SoundComponent);
	W_SAFE_DELETE(TextComponent);
	W_SAFE_DELETE(PhysicsComponent);
	W_SAFE_DELETE(Renderer);

	W_SAFE_DELETE(FileManager);
	W_SAFE_DELETE(TerrainManager);
	W_SAFE_DELETE(ParticlesManager);
	W_SAFE_DELETE(ObjectManager);
	W_SAFE_DELETE(SpriteManager);
	W_SAFE_DELETE(GeometryManager);
	W_SAFE_DELETE(MaterialManager);
	W_SAFE_DELETE(EffectManager);
	W_SAFE_DELETE(ShaderManager);
	W_SAFE_DELETE(CameraManager);
	W_SAFE_DELETE(RenderTargetManager);
	W_SAFE_DELETE(AnimationManager);
	W_SAFE_DELETE(ImageManager);
	W_SAFE_DELETE(LightManager);
	W_SAFE_DELETE(MemoryManager);

	W_SAFE_DELETE(WindowAndInputComponent);

	if (m_swapChainInitialized)
		m_swapChain.cleanup();
	m_swapChainInitialized = false;

	if (m_vkDevice)
		vkDestroyDevice(m_vkDevice, nullptr);
	m_vkDevice = VK_NULL_HANDLE;

#if (defined(DEBUG) || defined(_DEBUG))
	if (m_vkInstance) {
		PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
			reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
			(vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugReportCallbackEXT"));
		vkDestroyDebugReportCallbackEXT(m_vkInstance, m_debugCallback, nullptr);
	}
#endif

	if (m_vkInstance)
		vkDestroyInstance(m_vkInstance, nullptr);
	m_vkInstance = VK_NULL_HANDLE;
}

void Wasabi::SwitchState(WGameState* state) {
	if (curState) {
		curState->Cleanup();
	}
	curState = state;
	if (curState)
		curState->Load();
}

VkInstance Wasabi::CreateVKInstance() {
	/* Create Vulkan instance */
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = (const char*)engineParams["appName"];
	appInfo.pEngineName = W_ENGINE_NAME;
	appInfo.apiVersion = VK_API_VERSION_1_1;

	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
	std::vector<const char*> enabledLayers = {};

	// Enable surface extensions depending on os
#if defined(_WIN32)
	enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__linux__)
	enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#if (defined(DEBUG) || defined(_DEBUG))
	if ((bool)engineParams["enableVulkanValidation"])
		enabledLayers.push_back("VK_LAYER_LUNARG_standard_validation");
	enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	if (enabledExtensions.size() > 0) {
		instanceCreateInfo.enabledExtensionCount = (uint)enabledExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	}
	if (enabledLayers.size() > 0) {
		instanceCreateInfo.enabledLayerCount = enabledLayers.size();
		instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
	}

	VkInstance inst;
	VkResult r = vkCreateInstance(&instanceCreateInfo, nullptr, &inst);
	if (r != VK_SUCCESS)
		return nullptr;

#if (defined(DEBUG) || defined(_DEBUG))
	/* Load VK_EXT_debug_report entry points in debug builds */
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(inst, "vkCreateDebugReportCallbackEXT"));

	/* Setup callback creation information */
	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	callbackCreateInfo.pNext = nullptr;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	callbackCreateInfo.pfnCallback = &VulkanDebugReportCallback;
	callbackCreateInfo.pUserData = this;

	/* Register the callback */
	VkResult result = vkCreateDebugReportCallbackEXT(inst, &callbackCreateInfo, nullptr, &m_debugCallback);
#endif

	return inst;
}

WError Wasabi::StartEngine(int width, int height) {
	_DestroyResources();

	// This is created first so we can use its error message utility
	WindowAndInputComponent = CreateWindowAndInputComponent();

	// Create vulkan instance
	m_vkInstance = CreateVKInstance();
	if (!m_vkInstance)
		return WError(W_FAILEDTOCREATEINSTANCE);

	// Physical device
	uint gpuCount = 0;
	// Get number of available physical devices
	VkResult err = vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, nullptr);
	if (err != VK_SUCCESS || gpuCount == 0) {
		_DestroyResources();
		return WError(W_FAILEDTOLISTDEVICES);
	}

	// Enumerate devices
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	err = vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, physicalDevices.data());
	if (err != VK_SUCCESS)
		return WError(W_FAILEDTOLISTDEVICES);

	int index = SelectGPU(physicalDevices);
	if (index >= physicalDevices.size())
		index = 0;
	m_vkPhysDev = physicalDevices[index];

	// Find a queue that supports graphics operations
	uint graphicsQueueIndex = 0;
	uint queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysDev, &queueCount, NULL);
	if (queueCount == 0)
		return WError(W_FAILEDTOLISTDEVICES);

	std::vector<VkQueueFamilyProperties> queueProps;
	queueProps.resize(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysDev, &queueCount, queueProps.data());

	for (graphicsQueueIndex = 0; graphicsQueueIndex < queueCount; graphicsQueueIndex++) {
		if (queueProps[graphicsQueueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			break;
	}
	if (graphicsQueueIndex == queueCount)
		return WError(W_HARDWARENOTSUPPORTED);

	//
	// Create Vulkan device
	//
	std::array<float, 1> queuePriorities = { 0.0f };
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = queuePriorities.data();

	std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceFeatures features = GetDeviceFeatures();
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = NULL;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.pEnabledFeatures = &features;

	if (enabledExtensions.size() > 0) {
		deviceCreateInfo.enabledExtensionCount = (uint)enabledExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	}

	err = vkCreateDevice(m_vkPhysDev, &deviceCreateInfo, nullptr, &m_vkDevice);
	if (err != VK_SUCCESS)
		return WError(W_UNABLETOCREATEDEVICE);

	// Get the graphics queue
	vkGetDeviceQueue(m_vkDevice, graphicsQueueIndex, 0, &m_graphicsQueue);

	MemoryManager = new WVulkanMemoryManager();
	WError werr = MemoryManager->Initialize(m_vkPhysDev, m_vkDevice, m_graphicsQueue, graphicsQueueIndex);
	if (!werr)
		return werr;

	Renderer = new WRenderer(this);
	SoundComponent = CreateSoundComponent();
	TextComponent = CreateTextComponent();

	werr = WindowAndInputComponent->Initialize(width, height);
	if (!werr)
		return werr;

	m_swapChain.connect(m_vkInstance, m_vkPhysDev, m_vkDevice);
	if (!m_swapChain.initSurface(WindowAndInputComponent->GetVulkanSurface()))
		return WError(W_UNABLETOCREATESWAPCHAIN);
	m_swapChainInitialized = true;

	PhysicsComponent = CreatePhysicsComponent();

	FileManager = new WFileManager(this);
	ObjectManager = new WObjectManager(this);
	GeometryManager = new WGeometryManager(this);
	EffectManager = new WEffectManager(this);
	ShaderManager = new WShaderManager(this);
	MaterialManager = new WMaterialManager(this);
	CameraManager = new WCameraManager(this);
	ImageManager = new WImageManager(this);
	SpriteManager = new WSpriteManager(this);
	RenderTargetManager = new WRenderTargetManager(this);
	LightManager = new WLightManager(this);
	AnimationManager = new WAnimationManager(this);
	ParticlesManager = new WParticlesManager(this);
	TerrainManager = new WTerrainManager(this);

	if (!CameraManager->Load())
		return WError(W_ERRORUNK);

	werr = Renderer->Initialize();
	if (!werr)
		return werr;

	if (!ObjectManager->Load())
		return WError(W_ERRORUNK);
	if (!ImageManager->Load())
		return WError(W_ERRORUNK);
	if (!SpriteManager->Load())
		return WError(W_ERRORUNK);
	if (!LightManager->Load())
		return WError(W_ERRORUNK);
	if (!ParticlesManager->Load())
		return WError(W_ERRORUNK);
	if (!TerrainManager->Load())
		return WError(W_ERRORUNK);

	werr = SetupRenderer();
	if (!werr)
		return werr;

	if (TextComponent)
		werr = TextComponent->Initialize();
	if (!werr)
		return WError(W_ERRORUNK);

	return WError(W_SUCCEEDED);
}

WError Wasabi::Resize(unsigned int width, unsigned int height) {
	WError err = SpriteManager->Resize(width, height);
	if (!err)
		return err;
	return Renderer->Resize(width, height);
}

VkInstance Wasabi::GetVulkanInstance() const {
	return m_vkInstance;
}
VkPhysicalDevice Wasabi::GetVulkanPhysicalDevice() const {
	return m_vkPhysDev;
}
VkDevice Wasabi::GetVulkanDevice() const {
	return m_vkDevice;
}
VkQueue Wasabi::GetVulkanGraphicsQeueue() const {
	return m_graphicsQueue;
}

VulkanSwapChain* Wasabi::GetSwapChain() {
	return &m_swapChain;
}

int Wasabi::SelectGPU(std::vector<VkPhysicalDevice> devices) {
	return 0;
}

uint Wasabi::GetCurrentBufferingIndex() {
	return Renderer ? Renderer->GetCurrentBufferingIndex() : 0;
}

VkPhysicalDeviceFeatures Wasabi::GetDeviceFeatures() {
	VkPhysicalDeviceFeatures features = {};
	features.samplerAnisotropy = VK_TRUE;
	features.geometryShader = VK_TRUE;
	features.fillModeNonSolid = VK_TRUE;
	return features;
}

WError Wasabi::SetupRenderer() {
	return WInitializeDeferredRenderer(this);
}

WSoundComponent* Wasabi::CreateSoundComponent() {
	return nullptr;
}

WTextComponent* Wasabi::CreateTextComponent() {
	WTextComponent* tc = new WTextComponent(this);

#ifdef _WIN32
	char dir[MAX_PATH];
	int size = MAX_PATH;
	GetWindowsDirectoryA(dir, size);
	std::string s = dir;
	if (s[s.length() - 1] != '/' && s[s.length() - 1] != '\\')
		s += '/';
	tc->AddFontDirectory(s + "fonts");
#elif defined(__linux__)
#endif
	return tc;
}

WWindowAndInputComponent* Wasabi::CreateWindowAndInputComponent() {
	return new WGLFWWindowAndInputComponent(this);
}

WPhysicsComponent* Wasabi::CreatePhysicsComponent() {
	return nullptr;
}
