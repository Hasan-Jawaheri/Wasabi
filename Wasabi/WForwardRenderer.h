#pragma once

#include "Wasabi.h"

class WForwardRenderer : public WRenderer {
public:
	WForwardRenderer(Wasabi* app);
	~WForwardRenderer();

	virtual WError		Initiailize();
	virtual WError		Render();
	virtual void		Cleanup();
};
