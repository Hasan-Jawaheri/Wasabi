/** @file WWindowComponent.h
 *  @brief Window component spec
 *
 *  This defines the bare minimum requirements for a window component to be
 *  usable by Wasabi.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

/**
 * @ingroup engineclass
 *
 * This defines the bare minimum requirements for a window component to be
 * usable by Wasabi. A window component implements a platform-specific window
 * management utilities.
 */
class WWindowComponent {
public:
	WWindowComponent(class Wasabi* app) : m_app(app) {}

	/**
	 * Initializes the window component if it is not initialized, otherwise it
	 * should resize the window (or equivalent surface) to the new size provided.
	 * This function must be implemented by a child class to initialize its
	 * resources and create/resize the window (or equivalent surface) for
	 * rendering.
	 * @param  width  Width of the window to be created/resized to
	 * @param  height Height of the window to be created/resized to
	 * @return        Error code, see WError.h
	 */
	virtual WError Initialize(int width, int height) = 0;

	/**
	 * This function is called by Wasabi every frame to allow the window
	 * component to process any message pump or queue it has. This function must
	 * be implemented by a child class to perform any per-frame updates.
	 * @return true if the frame should be rendered, false otherwise (if the
	 *          window is not shown, e.g. minimized, this should return false)
	 */
	virtual bool Loop() = 0;

	/**
	 * Thus function is called by Wasabi before it exists to allow the window
	 * component to cleanup its resources. This function must be implemented by
	 * a child class.
	 */
	virtual void Cleanup() = 0;
	/**
	 * Retrieves the platform-specific handle needed by Vulkan. For example, on
	 * Windows, this must return the HINSTANCE of the application.
	 * @return The platform-specific application handle
	 */
	virtual void* GetPlatformHandle() const = 0;

	/**
	 * Retrieves the platform-specific window handle. For example, on Windows,
	 * this must return the HWND created by Initialize().
	 * @return The platform-specific window handle
	 */
	virtual void* GetWindowHandle() const = 0;

	/**
	 * Sets the title of the window.
	 * @param title New title for the window
	 */
	virtual void SetWindowTitle(const char* const title) {}

	/**
	 * Sets the position of the window on screen.
	 * @param x New x-coordinate of the window
	 * @param y New y-coordinate of the window
	 */
	virtual void SetWindowPosition(int x, int y) {}

	/**
	 * Sets the size of the window.
	 * @param width  New window width
	 * @param height New window height
	 */
	virtual void SetWindowSize(int width, int height) {}

	/**
	 * Maximizes the window.
	 */
	virtual void MaximizeWindow() {}

	/**
	 * Minimizes the window.
	 */
	virtual void MinimizeWindow() {}

	/**
	 * Restores the window (if it was minimized, it should make it appear).
	 * @return 0 if the window was hidden before this call, nonzero otherwise
	 */
	virtual uint RestoreWindow() { return -1; }

	/**
	 * Retrieves the width of the client area of the window.
	 * @return Width of the window's client area, in pixels
	 */
	virtual uint GetWindowWidth() const { return -1; }

	/**
	 * Retrieves the height of the client area of the window.
	 * @return Height of the window's client area, in pixels
	 */
	virtual uint GetWindowHeight() const { return -1; }

	/**
	 * Retrieves the x-coordinate of the window on the screen.
	 * @return X-coordinate of the window on the screen
	 */
	virtual int GetWindowPositionX() const { return 0; }

	/**
	 * Retrieves the y-coordinate of the window on the screen.
	 * @return Y-coordinate of the window on the screen
	 */
	virtual int GetWindowPositionY() const { return 0; }

	/**
	 * Sets the screen to full-screen or windowed.
	 * @param bFullScreen true to set the window to full-screen mode, false to
	 *                    set it to windowed mode
	 */
	virtual void SetFullScreenState(bool bFullScreen) {}

	/**
	 * Retrieves the full-screen state of the window, see SetFullScreenState().
	 * @return true if the window is in full-screen, false otherwise
	 */
	virtual bool GetFullScreenState() const { return false; }

	/**
	 * Sets the minimum size the window can have.
	 * @param minX Minimum width
	 * @param minY Minimum height
	 */
	virtual void SetWindowMinimumSize(int minX, int minY) {}

	/**
	 * Sets the maximum size the window can have.
	 * @param maxX Maximum width
	 * @param maxY Maximum height
	 */
	virtual void SetWindowMaximumSize(int maxX, int maxY) {}

protected:
	/** Pointer to the Wasabi class */
	class Wasabi* m_app;
};
