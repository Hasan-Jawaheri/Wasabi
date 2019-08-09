#include "WindowAndInput/GLFW/WGLFWWindowAndInputComponent.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#endif

#include "GLFW/glfw3native.h"

WGLFWWindowAndInputComponent::WGLFWWindowAndInputComponent(Wasabi* const app) : WWindowAndInputComponent(app) {
	m_window = nullptr;
	m_surface = nullptr;

	app->engineParams.insert(std::pair<std::string, void*>("defWndX", (void*)(-1))); // int
	app->engineParams.insert(std::pair<std::string, void*>("defWndY", (void*)(-1))); //int
}

WError WGLFWWindowAndInputComponent::Initialize(int width, int height) {
	if (!glfwInit())
		return WError(W_ERRORUNK);
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(width, height, "Wasabi", NULL, NULL);
	if (!m_window)
		return WError(W_WINDOWNOTCREATED);

	VkResult err = glfwCreateWindowSurface(m_app->GetVulkanInstance(), m_window, NULL, &m_surface);
	if (err != VK_SUCCESS) {
		glfwDestroyWindow(m_window);
		m_window = nullptr;
		return WError(W_WINDOWNOTCREATED);
	}

	return WError(W_SUCCEEDED);
}

bool WGLFWWindowAndInputComponent::Loop() {
	glfwPollEvents();
	bool shouldClose = glfwWindowShouldClose(m_window);
	if (shouldClose)
		m_app->__EXIT = true;
	return !shouldClose;
}

void WGLFWWindowAndInputComponent::Cleanup() {
	if (m_window) {
		glfwDestroyWindow(m_window);
		m_window = nullptr;
	}
	glfwTerminate();
}

void* WGLFWWindowAndInputComponent::GetPlatformHandle() const {
	return nullptr;
}

void* WGLFWWindowAndInputComponent::GetWindowHandle() const {
	return (void*)m_window;
}

VkSurfaceKHR WGLFWWindowAndInputComponent::GetVulkanSurface() const {
	return m_surface;
}

void WGLFWWindowAndInputComponent::ShowErrorMessage(std::string error, bool warning) {
#ifdef _WIN32
	HWND hWnd = glfwGetWin32Window(m_window);
	MessageBoxA(hWnd, error.c_str(), (LPCSTR)m_app->engineParams["appName"], MB_OK | (warning ? MB_ICONWARNING : MB_ICONERROR));
#endif
}

void WGLFWWindowAndInputComponent::SetWindowTitle(const char* const title) {

}

void WGLFWWindowAndInputComponent::SetWindowPosition(int x, int y) {

}

void WGLFWWindowAndInputComponent::SetWindowSize(int width, int height) {

}

void WGLFWWindowAndInputComponent::MaximizeWindow() {

}

void WGLFWWindowAndInputComponent::MinimizeWindow() {

}

uint WGLFWWindowAndInputComponent::RestoreWindow() {
	return 0;
}

uint WGLFWWindowAndInputComponent::GetWindowWidth() const {
	int w, h;
	glfwGetWindowSize(m_window, &w, &h);
	return (uint)w;
}

uint WGLFWWindowAndInputComponent::GetWindowHeight() const {
	int w, h;
	glfwGetWindowSize(m_window, &w, &h);
	return (uint)h;
}

int WGLFWWindowAndInputComponent::GetWindowPositionX() const {
	int x, y;
	glfwGetWindowPos(m_window, &x, &y);
	return x;
}

int WGLFWWindowAndInputComponent::GetWindowPositionY() const {
	int x, y;
	glfwGetWindowPos(m_window, &x, &y);
	return y;
}

void WGLFWWindowAndInputComponent::SetFullScreenState(bool bFullScreen) {

}

bool WGLFWWindowAndInputComponent::GetFullScreenState() const {
	return false;
}

void WGLFWWindowAndInputComponent::SetWindowMinimumSize(int minX, int minY) {

}

void WGLFWWindowAndInputComponent::SetWindowMaximumSize(int maxX, int maxY) {

}

bool WGLFWWindowAndInputComponent::MouseClick(W_MOUSEBUTTON button) const {
	return false;
}

int WGLFWWindowAndInputComponent::MouseX(W_MOUSEPOSTYPE posT, uint vpID) const {
	return 0;
}

int WGLFWWindowAndInputComponent::MouseY(W_MOUSEPOSTYPE posT, uint vpID) const {
	return 0;
}

int WGLFWWindowAndInputComponent::MouseZ() const {
	return 0;
}

bool WGLFWWindowAndInputComponent::MouseInScreen(W_MOUSEPOSTYPE posT, uint vpID) const {
	return false;
}


void WGLFWWindowAndInputComponent::SetMousePosition(uint x, uint y, W_MOUSEPOSTYPE posT) {

}

void WGLFWWindowAndInputComponent::SetMouseZ(int value) {

}

void WGLFWWindowAndInputComponent::ShowCursor(bool bShow) {

}

void WGLFWWindowAndInputComponent::EnableEscapeKeyQuit() {

}

void WGLFWWindowAndInputComponent::DisableEscapeKeyQuit() {

}

bool WGLFWWindowAndInputComponent::KeyDown(char key) const {
	return false;
}

void WGLFWWindowAndInputComponent::InsertRawInput(char key, bool state) {

}
