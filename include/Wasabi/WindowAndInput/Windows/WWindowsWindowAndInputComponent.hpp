/** @file WWindowsWindowAndInputComponent.h
 *  @brief Window/input component for Windows
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Wasabi/WindowAndInput/WWindowAndInputComponent.h"

/**
 * Creating an instance of this class creates the following engine parameters:
 * * "classStyle": Windows class style (Default is (void*)(CS_HREDRAW | CS_VREDRAW))
 * * "classIcon": Windows class icon (Default is (void*)(NULL))
 * * "classIcon_sm": Windows class small icon (Default is (void*)(NULL))
 * * "classCursor": Windows cursor to use (Default is (void*)(LoadCursorA(NULL, MAKEINTRESOURCEA(32512)))
 * * "menuName": Windows menu resource name (Default is (void*)(NULL))
 * * "menuProc": Window menu procedure function (Default is (void*)(NULL))
 * * "windowMenu": Window HMENU handle (Default is (void*)(NULL))
 * * "windowParent": Parent window (Default is (void*)(NULL))
 * * "windowStyle": Window style (Default is (void*)(WS_CAPTION | WS_OVERLAPPEDWINDOW | WS_VISIBLE))
 * * "windowStyleEx": Window extended style (Default is (void*)(WS_EX_OVERLAPPEDWINDOW))
 * * "defWndX": Window starting X position (Default is (void*)(-1))
 * * "defWndY": Window starting X position (Default is (void*)(-1))
 */
class WWindowsWindowAndInputComponent : public WWindowAndInputComponent {
	friend LRESULT CALLBACK hMainWndProc(
		HWND hWnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

public:
	WWindowsWindowAndInputComponent(class Wasabi* const app);

	virtual WError Initialize(int width, int height);
	virtual bool Loop();
	virtual void Cleanup();

	/**
	 * Retrieves the window (HWND) handle.
	 * @return Window handle
	 */
	HWND GetWindow() const;

	/**
	 * Retrieves the Windows application instance handle (HINSTANCE).
	 * @return Instance handle
	 */
	HINSTANCE GetInstance() const;

	/**
	 * Assigns a function to intercept window procedure callbacks. This
	 * callback will be called once before the default handler is run by
	 * Wasabi and once after.
	 * @param proc  A function pointer which will be called given the
	 *              Wasabi instance, window proc arguments and a boolean
	 *              set to true on the pre-processing call and false on
	 *              the second, after-processing call
	 */
	void SetWndProcCallback(LRESULT(CALLBACK *proc)(Wasabi*, HWND, UINT, WPARAM, LPARAM, bool));

	virtual void* GetPlatformHandle() const;
	virtual void* GetWindowHandle() const;
	virtual VkSurfaceKHR GetVulkanSurface() const;

	virtual void ShowErrorMessage(std::string error, bool warning = false);

	virtual void SetWindowTitle(const char* const title);
	virtual void SetWindowPosition(int x, int y);
	virtual void SetWindowSize(int width, int height);
	virtual void MaximizeWindow();
	virtual void MinimizeWindow();
	virtual uint32_t RestoreWindow();

	virtual uint32_t GetWindowWidth(bool framebuffer = true) const;
	virtual uint32_t GetWindowHeight(bool framebuffer = true) const;
	virtual int GetWindowPositionX() const;
	virtual int GetWindowPositionY() const;

	virtual void SetFullScreenState(bool bFullScreen);
	virtual bool GetFullScreenState() const;
	virtual void SetWindowMinimumSize(int minX, int minY);
	virtual void SetWindowMaximumSize(int maxX, int maxY);

	virtual bool MouseClick(W_MOUSEBUTTON button) const;
	virtual double MouseX(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT, uint32_t vpID = 0) const;
	virtual double MouseY(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT, uint32_t vpID = 0) const;
	virtual double MouseZ() const;
	virtual bool MouseInScreen(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT, uint32_t vpID = 0) const;

	virtual void SetMousePosition(double x, double y, W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT);
	virtual void SetMouseZ(double value);
	virtual void ShowCursor(bool bShow);
	virtual void SetCursorMotionMode(bool bEnable);

	virtual void SetQuitKeys(bool escape = true, bool cmdW = true);

	virtual bool KeyDown(uint32_t key) const;

	virtual void InsertRawInput(uint32_t key, bool state);

private:
	/** Window handle */
	HWND m_mainWindow;
	/** Instance handle */
	HINSTANCE m_hInstance;
	/** Vulkan surface */
	VkSurfaceKHR m_surface;
	/** Minimum window width */
	int m_minWindowX;
	/** Minimum window height */
	int m_minWindowY;
	/** Maximum window width */
	int m_maxWindowX;
	/** Maximum window height */
	int m_maxWindowY;
	/** true if the window is currently minimized, false otherwise */
	bool m_isMinimized;
	/** Optional additional callback to intercept the window procedure */
	LRESULT(CALLBACK *m_extra_proc) (Wasabi*, HWND, UINT, WPARAM, LPARAM, bool);

	/** true if mouse left button is clicked, false otherwise */
	bool m_leftClick;
	/** true if mouse right button is clicked, false otherwise */
	bool m_rightClick;
	/** true if mouse middle button is clicked, false otherwise */
	bool m_middleClick;
	/** Scroll value for the mouse */
	int m_mouseZ;
	/** true iff escape key quit is on */
	bool m_escapeE;
	/** states of all keys */
	bool m_keyDown[256];
};