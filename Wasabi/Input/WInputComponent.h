/** @file WInputComponent.h
 *  @brief Input component spec
 *
 *  This defines the bare minimum requirements for an input component to be
 *  usable by Wasabi.
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
 * This defines the bare minimum requirements for an input component to be
 * usable by Wasabi. An input component implements a platform-specific
 * input retrieval system.
 */
class WInputComponent {
public:
	WInputComponent(class Wasabi* const app) : m_app(app) {}

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
	 * @param x New mouse position x
	 * @param y New mouse position y
	 */
	virtual void SetMousePosition(uint x, uint y) = 0;

	/**
	 * Sets the value of the mouse scroll.
	 * @param value New value to set
	 */
	virtual void SetMouseZ(int value) = 0;

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
