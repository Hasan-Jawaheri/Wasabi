/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine core
*********************************************************************/

#pragma once

#include "Common.h"
#include "WWindowComponent.h"

class WCore {
public:
	class Wasabi* app;
	class WSoundComponent* SoundComponent;
	class WWindowComponent* WindowComponent;
	class WInputComponent* InputComponent;
	std::map<std::string, void*> engineParams;

	WCore();
	~WCore();

	WError Init(int width, int height);

	bool __EXIT; // when set to true, the engine will exit asap
};

#include "Wasabi.h"
