#pragma once

#include "WError.h"
#include "WCore.h"

class WGameState {
protected:
	class Wasabi* const app;
	WCore* const core;

public:
	WGameState(class  Wasabi* const a) : app(a), core(a->core) {}
	~WGameState(void) {}

	virtual void Load(void) {}
	virtual void Update(float fDeltaTime) {}
	virtual void OnKeydown(char c) {}
	virtual void OnKeyup(char c) {}
	virtual void OnInput(char c) {}
	virtual void Cleanup(void) {}
};

class Wasabi {
public:
	WCore* core;
	float FPS, maxFPS;
	WGameState* curState;

	virtual WError	Setup() = 0;
	virtual bool	Loop(float fDeltaTime) = 0;
	virtual void	Cleanup() = 0;

	void SwitchState(WGameState* state);
};

Wasabi* WInitialize();
