/** @file WIC_Win32.h
 *  @brief Input implementation for MS Windows
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../WInputComponent.h"

/**
 * @ingroup engineclass
 *
 * Implementation of the input component for Windows.
 */
class WIC_Win32 : public WInputComponent {
	friend LRESULT CALLBACK hMainWndProc(HWND hWnd, uint msg,
										 WPARAM wParam, LPARAM lParam);

public:
	WIC_Win32(class Wasabi* const app);

	virtual bool MouseClick(W_MOUSEBUTTON button) const;
	virtual int MouseX(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT,
					   uint vpID = 0) const;
	virtual int MouseY(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT,
					   uint vpID = 0) const;
	virtual int MouseZ() const;
	virtual bool MouseInScreen(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT,
							   uint vpID = 0) const;
	virtual void SetMousePosition(uint x, uint y);
	virtual void SetMouseZ(int value);
	virtual void EnableEscapeKeyQuit();
	virtual void DisableEscapeKeyQuit();
	virtual bool KeyDown(char key) const;

	virtual void InsertRawInput(char key, bool state);

protected:
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
