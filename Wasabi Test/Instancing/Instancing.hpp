#pragma once

#include "../TestSuite.hpp"

class InstancingDemo : public WGameState {
	WObject* character;
	WGeometry* geometry;
	WImage* texture;
	vector<WObject*> objectsV;

public:
	InstancingDemo(Wasabi* const app);

	virtual void Load();
	virtual void Update(float fDeltaTime);
	virtual void Cleanup();
};