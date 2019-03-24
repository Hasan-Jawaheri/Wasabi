#pragma once

#include "../TestSuite.hpp"
#include <Renderers/ForwardRenderer/WForwardRenderer.h>

class SpritesDemo : public WTestState {
	WSprite* m_sprites[3];

public:
	SpritesDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WRenderer* CreateRenderer() { return new WForwardRenderer(m_app); }
};
