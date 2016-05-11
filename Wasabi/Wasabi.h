#pragma once

#include "WError.h"
#include "WCore.h"

class Wasabi {
public:
	WCore* core;
	float FPS, maxFPS;
	class  WGameState* curState;

	virtual WError	Setup() = 0;
	virtual bool	Loop(float fDeltaTime) = 0;
	virtual void	Cleanup() = 0;

	void SwitchState(class WGameState* state);
};

class WGameState {
protected:
	Wasabi* const app;
	WCore* const core;

public:
	WGameState(Wasabi* const a) : app(a), core(a->core) {}
	~WGameState(void) {}

	virtual void Load(void) {}
	virtual void Update(float fDeltaTime) {}
	virtual void OnKeydown(char c) {}
	virtual void OnKeyup(char c) {}
	virtual void OnInput(char c) {}
	virtual void Cleanup(void) {}
};

Wasabi* WInitialize();
