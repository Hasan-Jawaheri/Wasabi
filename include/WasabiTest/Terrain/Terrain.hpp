#pragma once

#include "TestSuite.hpp"
#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderer.hpp>

class TerrainDemo : public WTestState {
	WTerrain* terrain;
	WObject* player;

	WVector3 playerPos;

public:
	TerrainDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();

	virtual WError SetupRenderer() { return WInitializeForwardRenderer(m_app); }
};
