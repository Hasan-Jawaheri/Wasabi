#include "Wasabi.h"
#include "WTimer.h"
#include "WSound.h"

/****************************************************************/
/*
 *
 * DUMMY USER CODE
 *
 */

//#include "WHavokPhysics.h"
class Kofta : public Wasabi {
	WSprite* spr;

protected:
	void SetupComponents() {
		Wasabi::SetupComponents();
		//PhysicsComponent = new WHavokPhysics(this);
	}

public:
	WError Setup() {
		this->maxFPS = 0;
		WError err = StartEngine(640, 480);

		WObject* o = new WObject(this);
		WObject* o2 = new WObject(this);
		WObject* o3 = new WObject(this);

		WGeometry* g = new WGeometry(this);
		g->CreateCone(1, 2, 10, 10);

		o->SetGeometry(g);
		o2->SetGeometry(g);
		o3->SetGeometry(g);

		g->RemoveReference();

		o->SetPosition(-5, 5, 0);
		o2->SetPosition(0, 0, -4);
		o3->SetPosition(5, 5, 0);
		o3->SetAngle(0, 0, 20);

		TextComponent->CreateFont(1, "FORTE");

		WImage* img = new WImage(this);
		img->Load("textures/dummy.bmp");
		o2->GetMaterial()->SetTexture(1, img);
		img->RemoveReference();

		spr = new WSprite(this);
		spr->SetImage(img);
		spr->SetRotationCenter(WVector2(128, 128));

		CameraManager->GetDefaultCamera()->Move(-10);

		return err;
	}
	bool Loop(float fDeltaTime) {
		char title[32];
		sprintf_s(title, 32, "FPS: %.2f", FPS);
		int mx = InputComponent->MouseX();
		int my = InputComponent->MouseY();
		int width = TextComponent->GetTextWidth(title, 32, 1);
		TextComponent->RenderText(title, mx - width / 2, my - 32, 32, 1);

		spr->Rotate(0.01f);
		spr->Move(0.1f);
		spr->SetAlpha(0.6f);

		return true;
	}
	void Cleanup() {

	}
};

Wasabi* WInitialize() {
	return new Kofta();
}

/****************************************************************/

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow) {
	WInitializeTimers();

	Wasabi* app = WInitialize();
	app->maxFPS = 60.0f;

	if (app->Setup()) {
		WTimer tmr(W_TIMER_SECONDS);
		WTimer fpsChangeTmr(W_TIMER_SECONDS);
		fpsChangeTmr.Start();
		UINT numFrames = 0;
		float deltaTime = 0.0f;
		while (!app->__EXIT) {
			tmr.Reset();
			tmr.Start();

			if (!app->WindowComponent->Loop())
				continue;

			if (deltaTime >= 0.00001f) {
				if (!app->Loop(deltaTime))
					break;
				if (app->curState)
					app->curState->Update(deltaTime);
				if (app->PhysicsComponent)
					app->PhysicsComponent->Step(deltaTime);
			}

			while (!app->Renderer->Render());

			numFrames++;

			deltaTime = app->FPS < 0.0001 ? 1.0f / 60.0f : 1.0f / app->FPS;

			if (app->maxFPS > 0.001) {
				float maxDeltaTime = 1.0f / app->maxFPS; // delta time at max FPS
				if (deltaTime < maxDeltaTime) {
					WTimer sleepTimer(W_TIMER_SECONDS);
					sleepTimer.Start();
					while (sleepTimer.GetElapsedTime() < (maxDeltaTime - deltaTime));
					deltaTime = maxDeltaTime;
				}
			}

			if (fpsChangeTmr.GetElapsedTime() >= 0.5f) //0.5 second passed -> update FPS
			{
				app->FPS = (float)numFrames * 2.0f;
				numFrames = 0;
				fpsChangeTmr.Reset();
			}
		}

		app->Cleanup();
	}

	W_SAFE_DELETE(app);

	WUnInitializeTimers();

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
	MessageBoxA(NULL, pMessage, "Vulkan Error", MB_OK | MB_ICONERROR);
	std::cerr << pMessage << std::endl;
	return VK_FALSE;
}

Wasabi::Wasabi() {
	engineParams = {
		{ "appName", (void*)"Wasabi" }, // LPCSTR
		{ "classStyle", (void*)(CS_HREDRAW | CS_VREDRAW) }, // DWORD
		{ "classIcon", (void*)(NULL) }, // HICON
		{ "classCursor", (void*)(LoadCursorA(NULL, MAKEINTRESOURCEA(32512))) }, // HCURSOR
		{ "menuName", (void*)(NULL) }, // LPCSTR
		{ "menuProc", (void*)(NULL) }, // void (*) (HMENU, UINT)
		{ "classIcon_sm", (void*)(NULL) }, // HICON
		{ "windowMenu", (void*)(NULL) }, // HMENU
		{ "windowParent", (void*)(NULL) }, // HWND
		{ "windowStyle", (void*)(WS_CAPTION | WS_OVERLAPPEDWINDOW | WS_VISIBLE) }, // DWORD
		{ "windowStyleEx", (void*)(WS_EX_OVERLAPPEDWINDOW) }, // DWORD
		{ "defAdapter", (void*)(0) }, // int
		{ "defWndX", (void*)(-1) }, // int
		{ "defWndY", (void*)(-1) }, //int
		{ "fontBmpSize", (void*)(512) }, // int
		{ "fontBmpCharHeight", (void*)(32) }, // int
		{ "fontBmpNumChars", (void*)(96) }, // int
		{ "textBatchSize", (void*)(256) }, // int
	};

	SoundComponent = nullptr;
	WindowComponent = nullptr;
	TextComponent = nullptr;
	PhysicsComponent = nullptr;
	Renderer = nullptr;

	ObjectManager = new WObjectManager(this);
	GeometryManager = new WGeometryManager(this);
	EffectManager = new WEffectManager(this);
	ShaderManager = new WShaderManager(this);
	MaterialManager = new WMaterialManager(this);
	CameraManager = new WCameraManager(this);
	ImageManager = new WImageManager(this);
	SpriteManager = new WSpriteManager(this);
}
Wasabi::~Wasabi() {
	if (WindowComponent)
		WindowComponent->Cleanup();
	if (Renderer)
		Renderer->Cleanup();
	if (PhysicsComponent)
		PhysicsComponent->Cleanup();

	W_SAFE_DELETE(SoundComponent);
	W_SAFE_DELETE(WindowComponent);
	W_SAFE_DELETE(TextComponent);
	W_SAFE_DELETE(PhysicsComponent);
	W_SAFE_DELETE(Renderer);

	W_SAFE_DELETE(ObjectManager);
	W_SAFE_DELETE(SpriteManager);
	W_SAFE_DELETE(GeometryManager);
	W_SAFE_DELETE(EffectManager);
	W_SAFE_DELETE(MaterialManager);
	W_SAFE_DELETE(ShaderManager);
	W_SAFE_DELETE(CameraManager);
	W_SAFE_DELETE(ImageManager);

	vkDestroyDevice(m_vkDevice, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
}

void Wasabi::SwitchState(WGameState* state) {
	if (curState) {
		curState->Cleanup();
		W_SAFE_DELETE(curState);
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
		instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
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
	SetupComponents();

	WError werr = WindowComponent->Initialize(width, height);
	if (!werr)
		return werr;

	/* Create vulkan instance */
	m_vkInstance = CreateVKInstance((const char*)engineParams["appName"], "Wasabi");
	if (!m_vkInstance)
		return WError(W_FAILEDTOCREATEINSTANCE);


	// Physical device
	uint32_t gpuCount = 0;
	// Get number of available physical devices
	VkResult err = vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, nullptr);
	if (err != VK_SUCCESS || gpuCount == 0) {
		vkDestroyInstance(m_vkInstance, nullptr);
		return WError(W_FAILEDTOLISTDEVICES);
	}

	// Enumerate devices
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	err = vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, physicalDevices.data());
	if (err != VK_SUCCESS) {
		vkDestroyInstance(m_vkInstance, nullptr);
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
	uint32_t graphicsQueueIndex = 0;
	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysDev, &queueCount, NULL);
	if (queueCount == 0) {
		vkDestroyInstance(m_vkInstance, nullptr);
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
		vkDestroyInstance(m_vkInstance, nullptr);
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
		deviceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	}

	err = vkCreateDevice(m_vkPhysDev, &deviceCreateInfo, nullptr, &m_vkDevice);
	if (err != VK_SUCCESS) {
		vkDestroyInstance(m_vkInstance, nullptr);
		return WError(W_UNABLETOCREATEDEVICE);
	}

	// Get the graphics queue
	vkGetDeviceQueue(m_vkDevice, graphicsQueueIndex, 0, &m_queue);

	// Store properties (including limits) and features of the phyiscal device
	vkGetPhysicalDeviceProperties(m_vkPhysDev, &m_deviceProperties);
	vkGetPhysicalDeviceFeatures(m_vkPhysDev, &m_deviceFeatures);
	// Gather physical device memory properties
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysDev, &m_deviceMemoryProperties);

	werr = Renderer->Initiailize();
	if (werr)
		return werr;

	CameraManager->Load();
	ImageManager->Load();
	SpriteManager->Load();

	if (TextComponent)
		TextComponent->Initialize();
	if (PhysicsComponent)
		werr = PhysicsComponent->Initialize();

	return werr;
}

WError Wasabi::Resize(int width, int height) {
	return Renderer->Resize(width, height);
}


void Wasabi::GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t * typeIndex) const {
	for (uint32_t i = 0; i < 32; i++) {
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

int Wasabi::SelectGPU(std::vector<VkPhysicalDevice> devices) {
	return 0;
}

void Wasabi::SetupComponents() {
	Renderer = new WForwardRenderer(this);
	SoundComponent = new WSoundComponent(this);
	TextComponent = new WTextComponent(this);

#ifdef _WIN32
	WindowComponent = new WWC_Win32(this);
	InputComponent = new WIC_Win32(this);
	char dir[MAX_PATH];
	int size = MAX_PATH;
	GetWindowsDirectoryA(dir, size);
	std::string s = dir;
	if (s[s.length() - 1] != '/' && s[s.length() - 1] != '\\')
		s += '/';
	TextComponent->AddFontDirectory(s + "fonts");
#elif defined(__linux__)
	WindowComponent = new WWC_Linux(this);
	InputComponent = new WIC_Linux(this);
#endif
}
