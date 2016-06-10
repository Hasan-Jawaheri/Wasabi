/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine renderer spec
*********************************************************************/
#pragma once

#include "Wasabi.h"

class WRenderer {
	Wasabi* m_app;

public:
	WRenderer(Wasabi* app) : m_app(app) {}
	~WRenderer() {}

	virtual WError		Initiailize() = 0;
	virtual WError		Render() = 0;
	virtual void		Cleanup() = 0;
};

