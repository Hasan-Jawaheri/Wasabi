/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine windows input component
*********************************************************************/

#pragma once

#include "../WInputComponent.h"

class WIC_Win32 : public WInputComponent {
	friend LRESULT CALLBACK hMainWndProc(HWND hWnd, uint msg, WPARAM wParam, LPARAM lParam);

public:
	WIC_Win32(class Wasabi* const app);

	virtual bool			MouseClick(W_MOUSEBUTTON button) const;
	virtual int				MouseX(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT, uint vpID = 0) const;
	virtual int				MouseY(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT, uint vpID = 0) const;
	virtual int				MouseZ() const;
	virtual bool			MouseInScreen(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT, uint vpID = 0) const;
	virtual void			SetMousePosition(uint x, uint y);
	virtual void			SetMouseZ(int value);
	virtual void			EnableEscapeKeyQuit();
	virtual void			DisableEscapeKeyQuit();
	virtual bool			KeyDown(char key) const;

	virtual void			InsertRawInput(char key, bool state);

protected:
	bool					m_leftClick, m_rightClick, m_middleClick;
	int						m_mouseX, m_mouseY, m_mouseZ;
	bool					m_escapeE;
	bool					m_keyDown[256];
};
