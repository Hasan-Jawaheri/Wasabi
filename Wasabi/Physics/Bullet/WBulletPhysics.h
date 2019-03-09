#pragma once

#include "../../Wasabi.h"

class WBulletPhysics : public WPhysicsComponent {
public:
	WBulletPhysics(class Wasabi* app);
	~WBulletPhysics();

	virtual WError Initialize();
	virtual void Cleanup();
	virtual void Start();
	virtual void Stop();
	virtual void Step(float deltaTime);
	virtual bool Stepping() const;
	virtual bool RayCast(WVector3 from, WVector3 to);
	virtual bool RayCast(WVector3 from, WVector3 to, W_RAYCAST_OUTPUT* out);

	void SetSpeed(float fSpeed);
	void SetGravity(float x, float y, float z);
	void SetGravity(WVector3 gravity);

private:
	struct WBulletPhysicsData* m_data;
};
