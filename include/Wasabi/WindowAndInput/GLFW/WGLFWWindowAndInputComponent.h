/** @file WWindowsWindowAndInputComponent.h
 *  @brief Window/input component using GLFW, supports Windows, Linux and Mac
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "WindowAndInput/WWindowAndInputComponent.h"

#include <GLFW/glfw3.h>

 /**
 * Creating an instance of this class creates the following engine parameters:
 * * "defWndX": Window starting X position (Default is (void*)(-1))
 * * "defWndY": Window starting X position (Default is (void*)(-1))
  */
class WGLFWWindowAndInputComponent : public WWindowAndInputComponent {
public:
	WGLFWWindowAndInputComponent(class Wasabi* const app);

	virtual WError Initialize(int width, int height);
	virtual bool Loop();
	virtual void Cleanup();

	virtual void* GetPlatformHandle() const;
	virtual void* GetWindowHandle() const;
	virtual VkSurfaceKHR GetVulkanSurface() const;

	virtual void ShowErrorMessage(std::string error, bool warning = false);

	virtual void SetWindowTitle(const char* const title);
	virtual void SetWindowPosition(int x, int y);
	virtual void SetWindowSize(int width, int height);
	virtual void MaximizeWindow();
	virtual void MinimizeWindow();
	virtual uint RestoreWindow();

	virtual uint GetWindowWidth() const;
	virtual uint GetWindowHeight() const;
	virtual int GetWindowPositionX() const;
	virtual int GetWindowPositionY() const;

	virtual void SetFullScreenState(bool bFullScreen);
	virtual bool GetFullScreenState() const;
	virtual void SetWindowMinimumSize(int minX, int minY);
	virtual void SetWindowMaximumSize(int maxX, int maxY);

	virtual bool MouseClick(W_MOUSEBUTTON button) const;
	virtual int MouseX(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT,
		uint vpID = 0) const;
	virtual int MouseY(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT,
		uint vpID = 0) const;
	virtual int MouseZ() const;
	virtual bool MouseInScreen(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT,
		uint vpID = 0) const;

	virtual void SetMousePosition(uint x, uint y, W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT);
	virtual void SetMouseZ(int value);
	virtual void ShowCursor(bool bShow);

	virtual void EnableEscapeKeyQuit();
	virtual void DisableEscapeKeyQuit();

	virtual bool KeyDown(char key) const;

	virtual void InsertRawInput(char key, bool state);

private:
	/** GLFW window */
	GLFWwindow* m_window;
	/** Vulkan surface */
	VkSurfaceKHR m_surface;
};