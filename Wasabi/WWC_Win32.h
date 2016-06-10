/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine window manager for win32
*********************************************************************/

#pragma once

#ifdef _WIN32
#include "Wasabi.h"

class WWC_Win32 : public WWindowComponent {
	friend LRESULT CALLBACK hMainWndProc(HWND hWnd, uint msg, WPARAM wParam, LPARAM lParam);
public:
	WWC_Win32(Wasabi* app);
	virtual WError Initialize(int width, int height);
	virtual bool Loop();
	virtual void Cleanup();

	virtual void SetWindowTitle(const char* const title);
	virtual void SetWindowPosition(int x, int y);
	virtual void SetWindowSize(int width, int height);
	virtual void MaximizeWindow(void);
	virtual void MinimizeWindow(void);
	virtual uint RestoreWindow(void);
	virtual uint GetWindowWidth(void) const;
	virtual uint GetWindowHeight(void) const;
	virtual int GetWindowPositionX(void) const;
	virtual int GetWindowPositionY(void) const;
	virtual void SetFullScreenState(bool bFullScreen);
	virtual bool GetFullScreenState(void) const;
	virtual void SetWindowMinimumSize(int minX, int minY);
	virtual void SetWindowMaximumSize(int maxX, int maxY);

	HWND GetWindow(void) const;
	HINSTANCE GetInstance(void) const;
	virtual void* GetPlatformHandle() const;
	virtual void* GetWindowHandle() const;

private:
	HWND						m_mainWindow;
	HINSTANCE					m_hInstance;
	int							m_minWindowX, m_minWindowY, m_maxWindowX, m_maxWindowY;
	bool						m_isMinimized;
};

#endif
