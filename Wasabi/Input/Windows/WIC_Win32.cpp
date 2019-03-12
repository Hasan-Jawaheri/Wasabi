#ifdef _WIN32

#include "WIC_Win32.h"
#include "../../Windows/Windows/WWC_Win32.h"
#include <windows.h>

WIC_Win32::WIC_Win32(Wasabi* const app) : WInputComponent(app) {
	m_rightClick = m_leftClick = m_middleClick = false;
	m_mouseZ = 0;
	m_escapeE = true;
	for (uint i = 0; i < 256; i++)
		m_keyDown[i] = false;
}

bool WIC_Win32::MouseClick(W_MOUSEBUTTON button) const {
	//return the registered click state

	if (button == MOUSE_LEFT)
		return m_leftClick;
	else if (button == MOUSE_RIGHT)
		return m_rightClick;
	else if (button == MOUSE_MIDDLE)
		return m_middleClick;

	return false;
}

int WIC_Win32::WIC_Win32::MouseX(W_MOUSEPOSTYPE posT, uint vpID) const {
	//get mouse position and convert it to the desired type
	RECT rc;
	POINT pt, __pt;
	GetWindowRect(((WWC_Win32*)m_app->WindowComponent)->GetWindow(), &rc);
	GetCursorPos(&__pt);
	GetCursorPos(&pt);
	ScreenToClient(((WWC_Win32*)m_app->WindowComponent)->GetWindow(), &pt);

	if (posT == MOUSEPOS_WINDOW || posT == MOUSEPOS_VIEWPORT)
		return pt.x; //default, window position
	/*else if (posT == MOUSEPOS_VIEWPORT) {
		//convert the mouse position to viewport space
		pt.x -= ((WWC_Win32*)m_app->WindowComponent)->GetViewportHandle(vpID)->vp.TopLeftX;

		return pt.x;
	}*/
	else if (posT == MOUSEPOS_DESKTOP)
		return __pt.x; //return desktop space position
	else
		return 0;

	return 0;
}

int WIC_Win32::MouseY(W_MOUSEPOSTYPE posT, uint vpID) const {
	//get mouse position and convert it to the desired type
	RECT rc;
	POINT pt, __pt;
	GetWindowRect(((WWC_Win32*)m_app->WindowComponent)->GetWindow(), &rc);
	GetCursorPos(&__pt);
	GetCursorPos(&pt);
	ScreenToClient(((WWC_Win32*)m_app->WindowComponent)->GetWindow(), &pt);

	if (posT == MOUSEPOS_WINDOW || posT == MOUSEPOS_VIEWPORT)
		return pt.y; //default, window position
	/*else if (posT == MOUSEPOS_VIEWPORT) {
		//convert the mouse position to viewport space
		pt.y -= m((WWC_Win32*)m_app->WindowComponent)->GetViewportHandle(vpID)->vp.TopLeftY;

		return pt.y;
	}*/
	else if (posT == MOUSEPOS_DESKTOP)
		return __pt.y; //return desktop space position
	else
		return 0;

	return 0;
}

int WIC_Win32::MouseZ() const {
	//return registered mouse wheel position
	return m_mouseZ;
}

bool WIC_Win32::MouseInScreen(W_MOUSEPOSTYPE posT, uint vpID) const {
	if (posT == MOUSEPOS_WINDOW) //check if mouse is in the window
	{
		POINT pt;
		GetCursorPos(&pt);
		RECT rc;
		GetWindowRect(((WWC_Win32*)m_app->WindowComponent)->GetWindow(), &rc);
		uint style = GetWindowLong(((WWC_Win32*)m_app->WindowComponent)->GetWindow(), GWL_STYLE);
		rc.left += (style & WS_BORDER ? 10 : 0);
		rc.top += (style & WS_CAPTION ? 30 : 0);
		rc.right -= (style & WS_BORDER ? 10 : 0);
		rc.bottom -= (style & WS_BORDER ? 10 : 0);

		//compare mouse position to window space
		if (pt.x > rc.left && pt.x < rc.right && pt.y > rc.top && pt.y < rc.bottom)
			return true;
	}
	else if (posT == MOUSEPOS_DESKTOP)
		return true; //the mouse is always in desktop
	/*else if (posT == MOUSEPOS_VIEWPORT)//check if the mouse is in a viewport
	{
		POINT pt;
		GetCursorPos(&pt);
		RECT rc;
		GetWindowRect(((WWC_Win32*)m_app->WindowComponent)->GetWindow(), &rc);
		uint style = GetWindowLong(((WWC_Win32*)m_app->WindowComponent)->GetWindow(), GWL_STYLE);
		rc.left += (style & WS_BORDER ? 10 : 0);
		rc.top += (style & WS_CAPTION ? 30 : 0);
		rc.top += (GetMenu(((WWC_Win32*)m_app->WindowComponent)->GetWindow()) == NULL ? 0 : 20);
		rc.right -= (style & WS_BORDER ? 10 : 0);
		rc.bottom -= (style & WS_BORDER ? 10 : 0);

		_hxViewport* vp = m((WWC_Win32*)m_app->WindowComponent)->GetViewportHandle(vpID);
		//compare to viewport coordianates
		if (pt.x > vp->vp.TopLeftX + rc.left &&
			pt.x < vp->vp.TopLeftX + vp->vp.Width + rc.left &&
			pt.y > vp->vp.TopLeftY + rc.top &&
			pt.y < vp->vp.TopLeftY + vp->vp.Height + rc.top)
			return true;
	}*/

	return false;
}

void WIC_Win32::SetMousePosition(uint x, uint y) {
	POINT pos;
	pos.x = x;
	pos.y = y;
	//convert to screen space
	ClientToScreen(((WWC_Win32*)m_app->WindowComponent)->GetWindow(), &pos);
	SetCursorPos(pos.x, pos.y);
}

void WIC_Win32::SetMouseZ(int value) {
	//set the mouse wheel position
	m_mouseZ = value;
}

void WIC_Win32::EnableEscapeKeyQuit() {
	m_escapeE = true;
}

void WIC_Win32::DisableEscapeKeyQuit() {
	m_escapeE = false;
}

bool WIC_Win32::KeyDown(char key) const {
	return m_keyDown[key];
}

void WIC_Win32::InsertRawInput(char key, bool state) {
	m_keyDown[key] = state;
}

#endif