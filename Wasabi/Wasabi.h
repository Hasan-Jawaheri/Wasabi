#pragma once

#include "WError.h"
#include "WCore.h"

class Wasabi {
public:
	WCore* core;

	virtual WError	Setup() = 0;
	virtual bool	Loop(float fDeltaTime) = 0;
	virtual void	Cleanup() = 0;
};


Wasabi* WInitialize();
