/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine physics implementation spec
*********************************************************************/

#pragma once

#include "Wasabi.h"

typedef struct W_RAYCAST_OUTPUT {
	float fDist;
	WVector3 normal;
} W_RAYCAST_OUTPUT;

class WPhysicsComponent {
public:
	WPhysicsComponent(Wasabi* app) : m_app(app) {}

	virtual WError		Initialize() = 0;
	virtual void		Cleanup() = 0;
	virtual void		Start() = 0;
	virtual void		Stop() = 0;
	virtual void		Step(float deltaTime) = 0;
	virtual bool		Stepping() const = 0;
	virtual bool		RayCast(WVector3 from, WVector3 to) = 0;
	virtual bool		RayCast(WVector3 from, WVector3 to, W_RAYCAST_OUTPUT* out) = 0;

protected:
	Wasabi* m_app;
};

