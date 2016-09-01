/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine generic orientation handler
*********************************************************************/

#pragma once

#include "WMath.h"

/*********************************************************************
Orientation handler
*********************************************************************/
enum STATE_CHANGE_TYPE { CHANGE_MOTION = 0, CHANGE_ROTATION = 1 };
class WOrientation {
public:
	WOrientation();
	~WOrientation();

	void			SetPosition(float x, float y, float z);
	void			SetPosition(const WVector3 pos);
	void			Point(float x, float y, float z);
	void			Point(WVector3 target);
	void			SetAngle(float x, float y, float z);
	void			SetAngle(WVector3 angle);
	void			SetAngle(WQuaternion quat);
	void			SetToRotation(const WOrientation* const device);
	void			SetULRVectors(WVector3 up, WVector3 look, WVector3 right);
	void			Yaw(float angle);
	void			Roll(float angle);
	void			Pitch(float angle);
	void			Move(float units);
	void			Strafe(float units);
	void			Fly(float units);

	float			GetPositionX() const;
	float			GetPositionY() const;
	float			GetPositionZ() const;
	WVector3		GetPosition() const;
	float			GetAngleX() const;
	float			GetAngleY() const;
	float			GetAngleZ() const;
	WVector3		GetAngle() const;
	WVector3		GetUVector() const;
	WVector3		GetLVector() const;
	WVector3		GetRVector() const;

	virtual void	SetBindingMatrix(WMatrix mtx);
	void			RemoveBinding();
	WMatrix			GetBindingMatrix() const;
	bool			IsBound() const;

	virtual void OnStateChange(STATE_CHANGE_TYPE type);

private:
	WVector3 m_pos;
	WVector3 m_right;
	WVector3 m_up;
	WVector3 m_look;
	bool m_bBind;
	WMatrix m_bindMtx;
};
