/** @file WWindowsWindowAndInputComponent.h
 *  @brief Window/input component for Windows
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../WWindowAndInputComponent.h"

/**
 * Creating an instance of this class creates the following engine parameters:
	app->engineParams.insert(std::pair<std::string, void*>("classStyle", (void*)(CS_HREDRAW | CS_VREDRAW)));
	app->engineParams.insert(std::pair<std::string, void*>("classStyle", (void*)(CS_HREDRAW | CS_VREDRAW))); // uint
	app->engineParams.insert(std::pair<std::string, void*>("classIcon", (void*)(NULL))); // HICON
	app->engineParams.insert(std::pair<std::string, void*>("classCursor", (void*)(LoadCursorA(NULL, MAKEINTRESOURCEA(32512))))); // HCURSOR
	app->engineParams.insert(std::pair<std::string, void*>("menuName", (void*)(NULL))); // LPCSTR
	app->engineParams.insert(std::pair<std::string, void*>("menuProc", (void*)(NULL))); // void (*) (HMENU, uint)
	app->engineParams.insert(std::pair<std::string, void*>("classIcon_sm", (void*)(NULL))); // HICON
	app->engineParams.insert(std::pair<std::string, void*>("windowMenu", (void*)(NULL))); // HMENU
	app->engineParams.insert(std::pair<std::string, void*>("windowParent", (void*)(NULL))); // HWND
	app->engineParams.insert(std::pair<std::string, void*>("windowStyle", (void*)(WS_CAPTION | WS_OVERLAPPEDWINDOW | WS_VISIBLE))); // uint
	app->engineParams.insert(std::pair<std::string, void*>("windowStyleEx", (void*)(WS_EX_OVERLAPPEDWINDOW))); // uint
	app->engineParams.insert(std::pair<std::string, void*>("defWndX", (void*)(-1))); // int
	app->engineParams.insert(std::pair<std::string, void*>("defWndY", (void*)(-1))); //int
 */
class WWindowsWindowAndInputComponent : public WWindowAndInputComponent {
	friend LRESULT CALLBACK hMainWndProc(
		HWND hWnd, uint msg, WPARAM wParam, LPARAM lParam);

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
	/** Window handle */
	HWND m_mainWindow;
	/** Instance handle */
	HINSTANCE m_hInstance;
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
	/** true if escape key quit is on, false otherwise */
	bool m_escapeE;
	/** states of all keys */
	bool m_keyDown[256];
};