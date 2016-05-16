#include "Wasabi.h"
#include "WTimer.h"
#include "WSound.h"

/****************************************************************/
/*
 *
 * DUMMY USER CODE
 *
 */

class Kofta : public Wasabi {
public:
	WError Setup() {
		WError err = StartEngine(500, 500);
		return err;
	}
	bool Loop(float fDeltaTime) {
		char title[32];
		sprintf_s(title, 32, "%f", FPS);
		WindowComponent->SetWindowTitle(title);
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

	app->SoundComponent = new WSoundComponent();

#ifdef _WIN32
	app->WindowComponent = new WWC_Win32(app);
	app->InputComponent = new WIC_Win32(app);
#elif defined(__linux__)
	app->WindowComponent = new WWC_Linux(app);
	app->InputComponent = new WIC_Linux(app);
#endif

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

			if (deltaTime) {
				if (!app->Loop(deltaTime))
					break;
				if (app->curState)
					app->curState->Update(deltaTime);
			}

			// TODO: render
			//while (app->core->Update(deltaTime) == HX_WINDOWMINIMIZED)
			//	hx->core->Loop();

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
	};
}
Wasabi::~Wasabi() {
	if (WindowComponent)
		WindowComponent->Cleanup();
	W_SAFE_DELETE(SoundComponent);
	W_SAFE_DELETE(WindowComponent);

	vkDestroyInstance(vkInstance, nullptr);
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

	// Enable surface extensions depending on os
#if defined(_WIN32)
	enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__linux__)
	enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	if (enabledExtensions.size() > 0) {
		instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	}

	VkInstance inst;
	VkResult r = vkCreateInstance(&instanceCreateInfo, nullptr, &inst);
	if (r != VK_SUCCESS)
		return nullptr;
	return inst;
}

WError Wasabi::StartEngine(int width, int height) {
	WError err = WindowComponent->Initialize(width, height);
	if (!err)
		return err;

	/* Create vulkan instance */
	vkInstance = CreateVKInstance((const char*)engineParams["appName"], "Wasabi");
	if (!vkInstance)
		return WError(W_FAILEDTOCREATEINSTANCE);


	/*// Physical device
	uint32_t gpuCount = 0;
	// Get number of available physical devices
	err = vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
	assert(!err);
	assert(gpuCount > 0);
	// Enumerate devices
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	err = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
	if (err) {
		vkTools::exitFatal("Could not enumerate phyiscal devices : \n" + vkTools::errorString(err), "Fatal error");
	}

	// Note :
	// This example will always use the first physical device reported,
	// change the vector index if you have multiple Vulkan devices installed
	// and want to use another one
	physicalDevice = physicalDevices[0];

	// Find a queue that supports graphics operations
	uint32_t graphicsQueueIndex = 0;
	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
	assert(queueCount >= 1);

	std::vector<VkQueueFamilyProperties> queueProps;
	queueProps.resize(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

	for (graphicsQueueIndex = 0; graphicsQueueIndex < queueCount; graphicsQueueIndex++) {
		if (queueProps[graphicsQueueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			break;
	}
	assert(graphicsQueueIndex < queueCount);

	// Vulkan device
	std::array<float, 1> queuePriorities = { 0.0f };
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = queuePriorities.data();

	err = createDevice(queueCreateInfo, enableValidation);
	assert(!err);

	// Store properties (including limits) and features of the phyiscal device
	// So examples can check against them and see if a feature is actually supported
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

#if defined(__ANDROID__)
	LOGD(deviceProperties.deviceName);
#endif

	// Gather physical device memory properties
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

	// Get the graphics queue
	vkGetDeviceQueue(device, graphicsQueueIndex, 0, &queue);

	// Find a suitable depth format
	VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(physicalDevice, &depthFormat);
	assert(validDepthFormat);

	swapChain.connect(instance, physicalDevice, device);

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queu
	err = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete);
	assert(!err);
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	err = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete);
	assert(!err);

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submitInfo = vkTools::initializers::submitInfo();
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphores.renderComplete;*/

	return WError(W_SUCCEEDED);
}

WError Wasabi::Resize(int width, int height) {
	return WError(W_SUCCEEDED);
}
