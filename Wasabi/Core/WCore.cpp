#include "../Core/WCore.h"
#include "../Renderers/WDeferredRenderer.h"
#include "../Windows/WWindowComponent.h"
#include "../Objects/WObject.h"
#include "../Geometries/WGeometry.h"
#include "../Materials/WEffect.h"
#include "../Materials/WEffect.h"
#include "../Cameras/WCamera.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"
#include "../Sprites/WSprite.h"
#include "../Lights/WLight.h"
#include "../Animations/WAnimation.h"
#include "../Physics/WPhysicsComponent.h"
#include "../Physics/Bullet/WBulletPhysics.h"
#include "../Sounds/WSound.h"
#include "../Texts/WText.h"
#include "../Particles/WParticles.h"
#include "../Terrains/WTerrain.h"

#ifdef _WIN32
#include "../Windows/Windows/WWC_Win32.h"
#include "../Input/Windows/WIC_Win32.h"
#endif

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow) {
#elif (defined __linux__)
int main() {
#endif
	Wasabi* app = WInitialize();
	int ret = RunWasabi(app);
	W_SAFE_DELETE(app);
	return ret;
}

int RunWasabi(Wasabi* app) {
	app->Timer.Start();
	if (app && app->Setup()) {
		unsigned int numFrames = 0;
		auto fpsTimer = std::chrono::high_resolution_clock::now();
		float maxFPSReached = app->maxFPS > 0.001f ? app->maxFPS : 60.0f;
		float deltaTime = 1.0f / maxFPSReached;
		while (!app->__EXIT) {
			auto tStart = std::chrono::high_resolution_clock::now();
			app->Timer.GetElapsedTime(true); // record elapsed time

			if (app->WindowComponent && !app->WindowComponent->Loop())
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
				app->Renderer->_Render();

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
#ifdef _WIN32
	MessageBoxA(NULL, pMessage, "Vulkan Error", MB_OK | MB_ICONERROR);
#elif (defined __linux__)
	/** TODO: Do a linux message box */
#endif
	std::cerr << pMessage << std::endl;
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
	};
	m_swapChainInitialized = false;

	SoundComponent = nullptr;
	WindowComponent = nullptr;
	TextComponent = nullptr;
	PhysicsComponent = nullptr;
	Renderer = nullptr;

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

	m_copyCommandBuffer = VK_NULL_HANDLE;

	curState = nullptr;
	__EXIT = false;
	maxFPS = 60.0f;
}
Wasabi::~Wasabi() {
	_DestroyResources();
}

void Wasabi::_DestroyResources() {
	if (WindowComponent)
		WindowComponent->Cleanup();
	if (Renderer)
		Renderer->_Cleanup();
	if (PhysicsComponent)
		PhysicsComponent->Cleanup();

	W_SAFE_DELETE(SoundComponent);
	W_SAFE_DELETE(WindowComponent);
	W_SAFE_DELETE(TextComponent);
	W_SAFE_DELETE(PhysicsComponent);
	W_SAFE_DELETE(Renderer);

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

	if (m_swapChainInitialized)
		m_swapChain.cleanup();
	m_swapChainInitialized = false;

	if (m_copyCommandBuffer)
		vkFreeCommandBuffers(m_vkDevice, m_cmdPool, 1, &m_copyCommandBuffer);
	m_copyCommandBuffer = VK_NULL_HANDLE;

	if (m_cmdPool)
		vkDestroyCommandPool(m_vkDevice, m_cmdPool, nullptr);
	m_cmdPool = VK_NULL_HANDLE;

	if (m_vkDevice)
		vkDestroyDevice(m_vkDevice, nullptr);
	if (m_vkInstance)
		vkDestroyInstance(m_vkInstance, nullptr);
	m_vkDevice = VK_NULL_HANDLE;
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

VkInstance CreateVKInstance(const char* appName, const char* engineName) {
	/* Create Vulkan instance */
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName;
	appInfo.pEngineName = engineName;
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
	std::vector<const char*> enabledLayers = {};

	// Enable surface extensions depending on os
#if defined(_WIN32)
	enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__linux__)
	enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#if (defined(DEBUG) || defined(_DEBUG))
	//enabledLayers.push_back("VK_LAYER_LUNARG_standard_validation");
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
	PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT =
		reinterpret_cast<PFN_vkDebugReportMessageEXT>
		(vkGetInstanceProcAddr(inst, "vkDebugReportMessageEXT"));
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(inst, "vkDestroyDebugReportCallbackEXT"));

	/* Setup callback creation information */
	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	callbackCreateInfo.pNext = nullptr;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	callbackCreateInfo.pfnCallback = &VulkanDebugReportCallback;
	callbackCreateInfo.pUserData = nullptr;

	/* Register the callback */
	VkDebugReportCallbackEXT callback;
	VkResult result = vkCreateDebugReportCallbackEXT(inst, &callbackCreateInfo, nullptr, &callback);
#endif

	return inst;
}

WError Wasabi::StartEngine(int width, int height) {
	/* Create vulkan instance */
	m_vkInstance = CreateVKInstance((const char*)engineParams["appName"], "Wasabi");
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
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_FAILEDTOLISTDEVICES);
	}

	// Note :
	// This example will always use the first physical device reported,
	// change the vector index if you have multiple Vulkan devices installed
	// and want to use another one
	int index = SelectGPU(physicalDevices);
	if (index >= physicalDevices.size())
		index = 0;
	m_vkPhysDev = physicalDevices[index];

	// Find a queue that supports graphics operations
	uint graphicsQueueIndex = 0;
	uint queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysDev, &queueCount, NULL);
	if (queueCount == 0) {
		_DestroyResources();
		return WError(W_FAILEDTOLISTDEVICES);
	}

	std::vector<VkQueueFamilyProperties> queueProps;
	queueProps.resize(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysDev, &queueCount, queueProps.data());

	for (graphicsQueueIndex = 0; graphicsQueueIndex < queueCount; graphicsQueueIndex++) {
		if (queueProps[graphicsQueueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			break;
	}
	if (graphicsQueueIndex == queueCount) {
		_DestroyResources();
		return WError(W_HARDWARENOTSUPPORTED);
	}

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

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = NULL;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.pEnabledFeatures = NULL;

	if (enabledExtensions.size() > 0) {
		deviceCreateInfo.enabledExtensionCount = (uint)enabledExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	}

	err = vkCreateDevice(m_vkPhysDev, &deviceCreateInfo, nullptr, &m_vkDevice);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_UNABLETOCREATEDEVICE);
	}

	// Get the graphics queue
	vkGetDeviceQueue(m_vkDevice, graphicsQueueIndex, 0, &m_queue);

	// Store properties (including limits) and features of the phyiscal device
	vkGetPhysicalDeviceProperties(m_vkPhysDev, &m_deviceProperties);
	vkGetPhysicalDeviceFeatures(m_vkPhysDev, &m_deviceFeatures);
	// Gather physical device memory properties
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysDev, &m_deviceMemoryProperties);

	Renderer = CreateRenderer();
	SoundComponent = CreateSoundComponent();
	TextComponent = CreateTextComponent();
	WindowComponent = CreateWindowComponent();
	InputComponent = CreateInputComponent();
	PhysicsComponent = CreatePhysicsComponent();

	WError werr = WindowComponent->Initialize(width, height);
	if (!werr) {
		_DestroyResources();
		return werr;
	}

	m_swapChain.connect(m_vkInstance, m_vkPhysDev, m_vkDevice);
	if (!m_swapChain.initSurface(
		WindowComponent->GetPlatformHandle(),
		WindowComponent->GetWindowHandle())) {
		_DestroyResources();
		return WError(W_UNABLETOCREATESWAPCHAIN);
	}
	m_swapChainInitialized = true;

	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = m_swapChain.queueNodeIndex;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	err = vkCreateCommandPool(m_vkDevice, &cmdPoolInfo, nullptr, &m_cmdPool);
	if (err != VK_SUCCESS) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

	VkCommandBufferAllocateInfo cmdBufInfo = {};
	// Buffer copies are done on the queue, so we need a command buffer for them
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufInfo.commandPool = m_cmdPool;
	cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufInfo.commandBufferCount = 1;

	err = vkAllocateCommandBuffers(m_vkDevice, &cmdBufInfo, &m_copyCommandBuffer);
	if (err) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
	}

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

	if (!CameraManager->Load()) {
		_DestroyResources();
		return WError(W_ERRORUNK);
	}

	werr = Renderer->_Initialize();
	if (!werr) {
		_DestroyResources();
		return werr;
	}

	if (!ObjectManager->Load()) {
		_DestroyResources();
		return WError(W_ERRORUNK);
	}
	if (!ImageManager->Load()) {
		_DestroyResources();
		return WError(W_ERRORUNK);
	}
	if (!SpriteManager->Load()) {
		_DestroyResources();
		return WError(W_ERRORUNK);
	}
	if (!LightManager->Load()) {
		_DestroyResources();
		return WError(W_ERRORUNK);
	}
	if (!ParticlesManager->Load()) {
		_DestroyResources();
		return WError(W_ERRORUNK);
	}
	if (!TerrainManager->Load()) {
		_DestroyResources();
		return WError(W_ERRORUNK);
	}

	if (TextComponent)
		werr = TextComponent->Initialize();
	if (!werr) {
		_DestroyResources();
		return WError(W_ERRORUNK);
	}

	werr = Renderer->LoadDependantResources();
	if (!werr) {
		_DestroyResources();
		return werr;
	}

	return WError(W_SUCCEEDED);
}

WError Wasabi::Resize(unsigned int width, unsigned int height) {
	return Renderer->Resize(width, height);
}

void Wasabi::GetMemoryType(uint typeBits, VkFlags properties, uint * typeIndex) const {
	for (uint i = 0; i < 32; i++) {
		if ((typeBits & 1) == 1) {
			if ((m_deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				*typeIndex = i;
				return;
			}
		}
		typeBits >>= 1;
	}
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
	return m_queue;
}

VulkanSwapChain* Wasabi::GetSwapChain() {
	return &m_swapChain;
}

VkCommandPool Wasabi::GetCommandPool() const {
	return m_cmdPool;
}

VkResult Wasabi::BeginCommandBuffer() {
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.pNext = NULL;

	VkResult err = vkResetCommandBuffer(m_copyCommandBuffer, 0);
	if (err)
		return err;

	// Put buffer region copies into command buffer
	// Note that the staging buffer must not be deleted before the copies
	// have been submitted and executed
	return vkBeginCommandBuffer(m_copyCommandBuffer, &cmdBufferBeginInfo);
}

VkResult Wasabi::EndCommandBuffer() {
	VkSubmitInfo copySubmitInfo = {};
	VkResult err = vkEndCommandBuffer(m_copyCommandBuffer);
	if (err)
		return err;

	// Submit copies to the queue
	copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers = &m_copyCommandBuffer;

	err = vkQueueSubmit(m_queue, 1, &copySubmitInfo, VK_NULL_HANDLE);
	if (err)
		return err;
	err = vkQueueWaitIdle(m_queue);
	if (err)
		return err;

	return VK_SUCCESS;
}

VkCommandBuffer Wasabi::GetCommandBuffer() const {
	return m_copyCommandBuffer;
}

int Wasabi::SelectGPU(std::vector<VkPhysicalDevice> devices) {
	return 0;
}

WRenderer* Wasabi::CreateRenderer() {
	return new WDeferredRenderer(this);
}

WSoundComponent* Wasabi::CreateSoundComponent() {
	return new WSoundComponent(this);
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

WWindowComponent* Wasabi::CreateWindowComponent() {
#ifdef _WIN32
	return new WWC_Win32(this);
#elif defined(__linux__)
	return new WWC_Linux(this);
#endif
}

WInputComponent* Wasabi::CreateInputComponent() {
#ifdef _WIN32
	return new WIC_Win32(this);
#elif defined(__linux__)
	return new WIC_Linux(this);
#endif
}

WPhysicsComponent* Wasabi::CreatePhysicsComponent() {
	WBulletPhysics* physics = new WBulletPhysics(this);
	WError werr = physics->Initialize();
	if (!werr)
		W_SAFE_DELETE(physics);
	return physics;
}
