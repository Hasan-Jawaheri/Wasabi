#include "WindowAndInput/GLFW/WGLFWWindowAndInputComponent.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#endif

#include <GLFW/glfw3.h>
#include "GLFW/glfw3native.h"

WGLFWWindowAndInputComponent::WGLFWWindowAndInputComponent(Wasabi* const app) : WWindowAndInputComponent(app) {
	m_window = nullptr;
	m_surface = nullptr;
	m_isMinimized = false;
	m_escapeQuit = true;
	m_leftClick = false;
	m_rightClick = false;
	m_middleClick = false;
	m_isMouseInScreen = false;
	m_mouseZ = 0;
	for (uint i = 0; i < sizeof(m_keyDown) / sizeof(m_keyDown[0]); i++)
		m_keyDown[i] = false;
	for (uint i = 0; i < 4; i++)
		m_windowSizeLimits[i] = GLFW_DONT_CARE;
}

WError WGLFWWindowAndInputComponent::Initialize(int width, int height) {
	if (!glfwInit())
		return WError(W_ERRORUNK);

	m_monitor = (void*)glfwGetPrimaryMonitor();
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = (void*)glfwCreateWindow(width, height, "Wasabi", NULL, NULL);
	if (!m_window)
		return WError(W_WINDOWNOTCREATED);
	glfwSetWindowUserPointer((GLFWwindow*)m_window, (void*)this);

	SetCallbacks();

	VkResult err = glfwCreateWindowSurface(m_app->GetVulkanInstance(), (GLFWwindow*)m_window, NULL, &m_surface);
	if (err != VK_SUCCESS) {
		glfwDestroyWindow((GLFWwindow*)m_window);
		m_window = nullptr;
		return WError(W_WINDOWNOTCREATED);
	}

	return WError(W_SUCCEEDED);
}

bool WGLFWWindowAndInputComponent::Loop() {
	glfwPollEvents();
	bool shouldClose = glfwWindowShouldClose((GLFWwindow*)m_window);
	if (shouldClose)
		m_app->__EXIT = true;
	return !shouldClose && !m_isMinimized;
}

void WGLFWWindowAndInputComponent::Cleanup() {
	if (m_window) {
		glfwDestroyWindow((GLFWwindow*)m_window);
		m_window = nullptr;
	}
	glfwTerminate();
}

void* WGLFWWindowAndInputComponent::GetPlatformHandle() const {
	return nullptr;
}

void* WGLFWWindowAndInputComponent::GetWindowHandle() const {
	return m_window;
}

VkSurfaceKHR WGLFWWindowAndInputComponent::GetVulkanSurface() const {
	return m_surface;
}

void WGLFWWindowAndInputComponent::ShowErrorMessage(std::string error, bool warning) {
#ifdef _WIN32
	HWND hWnd = glfwGetWin32Window((GLFWwindow*)m_window);
	MessageBoxA(hWnd, error.c_str(), m_app->GetEngineParam<LPCSTR>("appName"), MB_OK | (warning ? MB_ICONWARNING : MB_ICONERROR));
#endif
}

void WGLFWWindowAndInputComponent::SetWindowTitle(const char* const title) {
	glfwSetWindowTitle((GLFWwindow*)m_window, title);
}

void WGLFWWindowAndInputComponent::SetWindowPosition(int x, int y) {
	glfwSetWindowPos((GLFWwindow*)m_window, x, y);
}

void WGLFWWindowAndInputComponent::SetWindowSize(int width, int height) {
	glfwSetWindowSize((GLFWwindow*)m_window, width, height);
}

void WGLFWWindowAndInputComponent::MaximizeWindow() {
	glfwMaximizeWindow((GLFWwindow*)m_window);
}

void WGLFWWindowAndInputComponent::MinimizeWindow() {
	glfwIconifyWindow((GLFWwindow*)m_window);
}

uint WGLFWWindowAndInputComponent::RestoreWindow() {
	int ret = !m_isMinimized;
	glfwRestoreWindow((GLFWwindow*)m_window);
	return ret;
}

uint WGLFWWindowAndInputComponent::GetWindowWidth() const {
	int w, h;
	glfwGetWindowSize((GLFWwindow*)m_window, &w, &h);
	return (uint)w;
}

uint WGLFWWindowAndInputComponent::GetWindowHeight() const {
	int w, h;
	glfwGetWindowSize((GLFWwindow*)m_window, &w, &h);
	return (uint)h;
}

int WGLFWWindowAndInputComponent::GetWindowPositionX() const {
	int x, y;
	glfwGetWindowPos((GLFWwindow*)m_window, &x, &y);
	return x;
}

int WGLFWWindowAndInputComponent::GetWindowPositionY() const {
	int x, y;
	glfwGetWindowPos((GLFWwindow*)m_window, &x, &y);
	return y;
}

void WGLFWWindowAndInputComponent::SetFullScreenState(bool bFullScreen) {
	if (GetFullScreenState() == bFullScreen)
		return;

	if (bFullScreen) {
		// backup windwo position and window size
		glfwGetWindowPos((GLFWwindow*)m_window, &m_storedWindowDimensions[0], &m_storedWindowDimensions[1]);
		glfwGetWindowSize((GLFWwindow*)m_window, &m_storedWindowDimensions[2], &m_storedWindowDimensions[3]);

		const GLFWvidmode* mode = glfwGetVideoMode((GLFWmonitor*)m_monitor);
		glfwSetWindowMonitor((GLFWwindow*)m_window, (GLFWmonitor*)m_monitor, 0, 0, mode->width, mode->height, 0);
	} else {
		glfwSetWindowMonitor((GLFWwindow*)m_window, nullptr, m_storedWindowDimensions[0], m_storedWindowDimensions[1], m_storedWindowDimensions[2], m_storedWindowDimensions[3], 0);
	}
}

bool WGLFWWindowAndInputComponent::GetFullScreenState() const {
	return glfwGetWindowMonitor((GLFWwindow*)m_window) != nullptr;
}

void WGLFWWindowAndInputComponent::SetWindowMinimumSize(int minX, int minY) {
	m_windowSizeLimits[0] = minX;
	m_windowSizeLimits[1] = minY;
	glfwSetWindowSizeLimits((GLFWwindow*)m_window, m_windowSizeLimits[0], m_windowSizeLimits[1], m_windowSizeLimits[2], m_windowSizeLimits[3]);
}

void WGLFWWindowAndInputComponent::SetWindowMaximumSize(int maxX, int maxY) {
	m_windowSizeLimits[2] = maxX;
	m_windowSizeLimits[3] = maxY;
	glfwSetWindowSizeLimits((GLFWwindow*)m_window, m_windowSizeLimits[0], m_windowSizeLimits[1], m_windowSizeLimits[2], m_windowSizeLimits[3]);
}

bool WGLFWWindowAndInputComponent::MouseClick(W_MOUSEBUTTON button) const {
	if (button == MOUSE_LEFT)
		return m_leftClick;
	else if (button == MOUSE_RIGHT)
		return m_rightClick;
	else if (button == MOUSE_MIDDLE)
		return m_middleClick;
	return false;
}

int WGLFWWindowAndInputComponent::MouseX(W_MOUSEPOSTYPE posT, uint vpID) const {
	double mx, my;
	glfwGetCursorPos((GLFWwindow*)m_window, &mx, &my);
	if (posT == MOUSEPOS_VIEWPORT)
		return (int)mx;
	else if (posT == MOUSEPOS_DESKTOP) {
		int wx, wy;
		glfwGetWindowPos((GLFWwindow*)m_window, &wx, &wy);
		return mx + wx;
	}
	return 0;
}

int WGLFWWindowAndInputComponent::MouseY(W_MOUSEPOSTYPE posT, uint vpID) const {
	double mx, my;
	glfwGetCursorPos((GLFWwindow*)m_window, &mx, &my);
	if (posT == MOUSEPOS_VIEWPORT)
		return (int)my;
	else if (posT == MOUSEPOS_DESKTOP) {
		int wx, wy;
		glfwGetWindowPos((GLFWwindow*)m_window, &wx, &wy);
		return my + wy;
	}
	return 0;
}

int WGLFWWindowAndInputComponent::MouseZ() const {
	return m_mouseZ;
}

bool WGLFWWindowAndInputComponent::MouseInScreen(W_MOUSEPOSTYPE posT, uint vpID) const {
	return m_isMouseInScreen;
}

void WGLFWWindowAndInputComponent::SetMousePosition(uint x, uint y, W_MOUSEPOSTYPE posT) {
	if (posT == MOUSEPOS_VIEWPORT)
		glfwSetCursorPos((GLFWwindow*)m_window, (double)x, (double)y);
	else if (posT == MOUSEPOS_DESKTOP) {
		int wx, wy;
		glfwGetWindowPos((GLFWwindow*)m_window, &wx, &wy);
		glfwSetCursorPos((GLFWwindow*)m_window, (double)(x - wx), (double)(y - wy));
	}
}

void WGLFWWindowAndInputComponent::SetMouseZ(int value) {
	m_mouseZ = value;
}

void WGLFWWindowAndInputComponent::ShowCursor(bool bShow) {
	if (bShow)
		glfwSetInputMode((GLFWwindow*)m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else
		glfwSetInputMode((GLFWwindow*)m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void WGLFWWindowAndInputComponent::EnableEscapeKeyQuit() {
	m_escapeQuit = true;
}

void WGLFWWindowAndInputComponent::DisableEscapeKeyQuit() {
	m_escapeQuit = false;
}

bool WGLFWWindowAndInputComponent::KeyDown(unsigned int key) const {
	if (key >= 350)
		return false;
	return m_keyDown[key];
}

void WGLFWWindowAndInputComponent::InsertRawInput(unsigned int key, bool state) {
	if (key < 350)
		m_keyDown[key] = state;
}

void WGLFWWindowAndInputComponent::SetCallbacks() {
	glfwSetCharCallback((GLFWwindow*)m_window, [](GLFWwindow* window, unsigned int c) {
		WGLFWWindowAndInputComponent* comp = (WGLFWWindowAndInputComponent*)glfwGetWindowUserPointer(window);
		if (comp->m_app->curState)
			comp->m_app->curState->OnInput(c);
	});

	glfwSetKeyCallback((GLFWwindow*)m_window, [](GLFWwindow* window, int key, int scancode, int mode, int mods) {
		WGLFWWindowAndInputComponent* comp = (WGLFWWindowAndInputComponent*)glfwGetWindowUserPointer(window);
		if (mode == GLFW_RELEASE) {
			comp->InsertRawInput(key, false);
			if (comp->m_app->curState)
				comp->m_app->curState->OnKeyUp(key);
		} else if (mode == GLFW_PRESS) {
			comp->InsertRawInput(key, true);
			if (key == W_KEY_ESCAPE && comp->m_escapeQuit)
				glfwSetWindowShouldClose(window, 1);
			if (comp->m_app->curState)
				comp->m_app->curState->OnKeyDown(key);
		} else if (mode == GLFW_REPEAT) {}
	});
	
	glfwSetFramebufferSizeCallback((GLFWwindow*)m_window, [](GLFWwindow* window, int w, int h) {
		WGLFWWindowAndInputComponent* comp = (WGLFWWindowAndInputComponent*)glfwGetWindowUserPointer(window);
		if (w > 0 && h > 0)
			comp->m_app->Resize(w, h);
	});

	glfwSetWindowMaximizeCallback((GLFWwindow*)m_window, [](GLFWwindow* window, int isMaximized) {
		WGLFWWindowAndInputComponent* comp = (WGLFWWindowAndInputComponent*)glfwGetWindowUserPointer(window);
		comp->m_isMinimized = false;
	});

	glfwSetWindowIconifyCallback((GLFWwindow*)m_window, [](GLFWwindow* window, int isMinimized) {
		WGLFWWindowAndInputComponent* comp = (WGLFWWindowAndInputComponent*)glfwGetWindowUserPointer(window);
		comp->m_isMinimized = isMinimized;
	});

	glfwSetMouseButtonCallback((GLFWwindow*)m_window, [](GLFWwindow* window, int button, int mode, int mods) {
		WGLFWWindowAndInputComponent* comp = (WGLFWWindowAndInputComponent*)glfwGetWindowUserPointer(window);
		if (button == GLFW_MOUSE_BUTTON_1)
			comp->m_leftClick = mode == GLFW_PRESS;
		else if (button == GLFW_MOUSE_BUTTON_2)
			comp->m_rightClick = mode == GLFW_PRESS;
		else if (button == GLFW_MOUSE_BUTTON_3)
			comp->m_middleClick = mode == GLFW_PRESS;
	});

	glfwSetCursorEnterCallback((GLFWwindow*)m_window, [](GLFWwindow* window, int entered) {
		WGLFWWindowAndInputComponent* comp = (WGLFWWindowAndInputComponent*)glfwGetWindowUserPointer(window);
		comp->m_isMouseInScreen = entered == GLFW_TRUE;
	});

	glfwSetScrollCallback((GLFWwindow*)m_window, [](GLFWwindow* window, double x, double y) {
		WGLFWWindowAndInputComponent* comp = (WGLFWWindowAndInputComponent*)glfwGetWindowUserPointer(window);
		comp->m_mouseZ += y;
	});
}
