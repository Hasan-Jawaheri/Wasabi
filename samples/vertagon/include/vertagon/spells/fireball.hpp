#pragma once

#include "vertagon/common.hpp"
#include "vertagon/spells/spells.hpp"

class Spell_Fireball : public Spell {
public:
    Spell_Fireball(Vertagon* app);

    virtual WError Load() override;
    virtual bool Update(float fDeltaTime) override;
    virtual void Cleanup() override;
};
