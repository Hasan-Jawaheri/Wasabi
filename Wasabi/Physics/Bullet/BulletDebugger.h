#pragma once

#include "WBulletPhysics.h"
#include <thread>
#include <vector>
#include <mutex>

class BulletDebugger : public Wasabi, public btIDebugDraw {
	struct LINE {
		WVector3 from, to;
		WColor color;
	};

public:
	/** This is the physics component running in the main thread, not this one */
	WBulletPhysics* m_physics;
	bool m_keep_running;
	std::thread m_thread;

	int m_debugMode;

	float fYaw, fPitch, fDist;
	WVector3 vPos;

	class WObject* m_linesDrawer;
	std::vector<BulletDebugger::LINE> m_lines[2];
	std::mutex m_linesLock;
	int m_curLines;

	void ApplyMousePivot();

	BulletDebugger(WBulletPhysics* physics);

	virtual WError Setup();
	virtual bool Loop(float fDeltaTime);
	virtual void Cleanup();

	virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color);
	virtual void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color);
	virtual void reportErrorWarning(const char *warningString);
	virtual void draw3dText(const btVector3 &location, const char *textString);
	virtual void setDebugMode(int debugMode);
	virtual int getDebugMode() const;
	virtual void clearLines();

	virtual WRenderer* CreateRenderer();
	virtual WPhysicsComponent* CreatePhysicsComponent();

	static void Thread(void* debugger_ptr);
};