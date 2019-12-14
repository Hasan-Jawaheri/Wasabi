#pragma once

#include "vertagon/common.hpp"
#include "vertagon/spells/spells.hpp"

class Spell_Fireball : public Spell {
    WParticles* m_particles;
    WPointLight* m_light;

    float m_shotTime;
	bool m_firstPosition;
    bool m_exploded;
    WVector3 m_targetOrigin;
    WVector3 m_targetDirection;

    struct {
        float lifetime;
        float growthPeriod;
        float emissionPeriod;
        float idleTrailSpeed;
        float speed;
    } m_params;

public:
    Spell_Fireball(Vertagon* app);

    virtual WError Load() override;
    virtual void Update(float fDeltaTime) override;
    virtual void Cleanup() override;
    virtual bool IsAlive() override;

    void SetIdlePosition(WVector3 position, WVector3 up);
    void SetDirection(WVector3 origin, WVector3 direction);
};
