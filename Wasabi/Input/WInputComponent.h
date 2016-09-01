/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine input component spec
*********************************************************************/

#pragma once

#include "../Wasabi.h"

enum W_MOUSEBUTTON { MOUSE_LEFT = 0, MOUSE_RIGHT = 1, MOUSE_MIDDLE = 2 };
enum W_MOUSEPOSTYPE { MOUSEPOS_DESKTOP = 0, MOUSEPOS_WINDOW = 1, MOUSEPOS_VIEWPORT = 2 };

class WInputComponent {
public:
	WInputComponent(Wasabi* const app) : m_app(app) {}

	virtual bool			MouseClick(W_MOUSEBUTTON button) const = 0;
	virtual int				MouseX(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT, uint vpID = 0) const = 0;
	virtual int				MouseY(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT, uint vpID = 0) const = 0;
	virtual int				MouseZ() const = 0;
	virtual bool			MouseInScreen(W_MOUSEPOSTYPE posT = MOUSEPOS_VIEWPORT, uint vpID = 0) const = 0;
	virtual void			SetMousePosition(uint x, uint y) = 0;
	virtual void			SetMouseZ(int value) = 0;
	virtual void			EnableEscapeKeyQuit() = 0;
	virtual void			DisableEscapeKeyQuit() = 0;
	virtual bool			KeyDown(char key) const = 0;

	virtual void			InsertRawInput(char key, bool state) = 0;

protected:
	Wasabi*					m_app;
};
