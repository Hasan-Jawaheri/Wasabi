/** @file WWC_Win32.h
 *  @brief Window implementation for MS Windows
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#ifdef _WIN32
#include "../WWindowComponent.h"

/**
 * @ingrouop engineclass
 *
 * Implementation of the window component for Windows.
 */
class WWC_Win32 : public WWindowComponent {
	friend LRESULT CALLBACK hMainWndProc(
		HWND hWnd, uint msg, WPARAM wParam, LPARAM lParam);
public:
	WWC_Win32(Wasabi* app);
	virtual WError Initialize(int width, int height);
	virtual bool Loop();
	virtual void Cleanup();

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
};

#endif
