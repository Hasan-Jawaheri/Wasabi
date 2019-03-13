/** @file WWindowAndInputComponent.h
 *  @brief Window/input component interface
 *
 *  This is the interface to define a window/input component for Wasabi.
 *  A window/input component is responsible for platform-specific window and
 *  input manipulation and notifications.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

/**
 * @ingroup engineclass
 * Describes a mouse click button.
 */
enum W_MOUSEBUTTON {
	MOUSE_LEFT = 0,
	MOUSE_RIGHT = 1,
	MOUSE_MIDDLE = 2
};

/**
 * @ingroup engineclass
 * Describes the coordinate system for a mouse position.
 */
enum W_MOUSEPOSTYPE {
	MOUSEPOS_DESKTOP = 0,
	MOUSEPOS_WINDOW = 1,
	MOUSEPOS_VIEWPORT = 2
};

/**
 * @ingroup engineclass
 *
 * This defines the bare minimum requirements for an window/input component to be
 * usable by Wasabi.
 */
class WWindowAndInputComponent {
public:
	WWindowAndInputComponent(class Wasabi* const app) : m_app(app) {}

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
	 * Displays a platform-specific error or warning message.
	 * @param error   Error message to display
	 * @param warning whether or not to show a warning or error
	 */
	virtual void ShowErrorMessage(std::string error, bool warning = false) = 0;

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

	/**
	 * Check if a mouse button is clicked.
	 * @param  button Button to check
	 * @return        true if button is clicked, false otherwise
	 */
	virtual bool MouseClick(W_MOUSEBUTTON button) const = 0;

	/**
	 * Retrieves the x position of the system cursor.
	 * @param  posT The coordinate system which the position should be relative
	 *              to
	 * @param  vpID Unused
	 * @return      The x coordinate of the system cursor, relative to posT
	 */
	virtual int MouseX(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT,
					   uint vpID = 0) const = 0;

	/**
	 * Retrieves the y position of the system cursor.
	 * @param  posT The coordinate system which the position should be relative
	 *              to
	 * @param  vpID Unused
	 * @return      The y coordinate of the system cursor, relative to posT
	 */
	virtual int MouseY(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT,
					   uint vpID = 0) const = 0;

	/**
	 * Retrieves the current scroll value of the mouse wheel. When the mouse
	 * wheel is spun, each tick will increment or decrement this value.
	 * @return Current scroll value of the mouse wheel
	 */
	virtual int MouseZ() const = 0;

	/**
	 * Checks if the cursor is within the coordinate system posT.
	 * @param  posT Coordinate system to check if the cursor is inside
	 * @param  vpID Unused
	 * @return      true if the cursor is within the coordinate system posT,
	 *              false otherwise
	 */
	virtual bool MouseInScreen(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT,
							   uint vpID = 0) const = 0;

	/**
	 * Sets the cursor position, relative to the rendering viewport.
	 * @param x    New mouse position x
	 * @param y    New mouse position y
	 * @param posT The coordinate system which the position should be relative
	 *              to
	 */
	virtual void SetMousePosition(uint x, uint y, W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT) = 0;

	/**
	 * Sets the value of the mouse scroll.
	 * @param value New value to set
	 */
	virtual void SetMouseZ(int value) = 0;

	/**
	 * Shows or hides the mouse cursor.
	 * @param bShow  Whether to show or hide the cursor
	 */
	virtual void ShowCursor(bool bShow) = 0;

	/**
	 * Enables escape key quit. When escape key quit is enabled, the application
	 * will close when the escape key is captured.
	 */
	virtual void EnableEscapeKeyQuit() = 0;

	/**
	 * Disables escape key quit. When escape key quit is enabled, the application
	 * will close when the escape key is captured.
	 */
	virtual void DisableEscapeKeyQuit() = 0;

	/**
	 * Checks if a key is currently pressed.
	 * @param  key Key to check
	 * @return     true if key is pressed, false otherwise
	 */
	virtual bool KeyDown(char key) const = 0;

	/**
	 * Forcefully sets the state of a key.
	 * @param key   Key to set
	 * @param state State to set the key to, true being pressed and false being
	 *              released
	 */
	virtual void InsertRawInput(char key, bool state) = 0;

protected:
	/** Pointer to the owner Wasabi class */
	class Wasabi*			m_app;
};
