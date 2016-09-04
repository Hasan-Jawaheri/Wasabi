/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine window manager spec
*********************************************************************/

#pragma once

#include "../Core/WCore.h"

class WWindowComponent {
public:
	WWindowComponent(class Wasabi* app) : m_app(app) {}

	virtual WError Initialize(int width, int height) = 0;
	virtual bool Loop() = 0;
	virtual void Cleanup() = 0;
	virtual void* GetPlatformHandle() const = 0;
	virtual void* GetWindowHandle() const = 0;

	virtual void SetWindowTitle(const char* const title) {}
	virtual void SetWindowPosition(int x, int y) {}
	virtual void SetWindowSize(int width, int height) {}
	virtual void MaximizeWindow(void) {}
	virtual void MinimizeWindow(void) {}
	virtual uint RestoreWindow(void) { return -1; }
	virtual uint GetWindowWidth(void) const { return -1; }
	virtual uint GetWindowHeight(void) const { return -1; }
	virtual int GetWindowPositionX(void) const { return 0; }
	virtual int GetWindowPositionY(void) const { return 0; }
	virtual void SetFullScreenState(bool bFullScreen) {}
	virtual bool GetFullScreenState(void) const { return true; }
	virtual void SetWindowMinimumSize(int minX, int minY) {}
	virtual void SetWindowMaximumSize(int maxX, int maxY) {}

protected:
	class Wasabi* m_app;
};
