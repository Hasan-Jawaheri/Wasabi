#pragma once

#include "vertagon/common.hpp"
#include "vertagon/spells/spells.hpp"

class Spell_Explosion : public Spell {
    WParticles* m_particles;
    WPointLight* m_light;

    float m_startTime;
    WVector3 m_position;

    struct {
        float lifetime;
        float peakLightRange;
        float peakIntensity;
    } m_params;

public:
    Spell_Explosion(Vertagon* app, WVector3 position);

    virtual WError Load() override;
    virtual void Update(float fDeltaTime) override;
    virtual void Cleanup() override;
    virtual bool IsAlive() override;
};
