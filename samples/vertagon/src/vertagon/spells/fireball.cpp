#include "vertagon/spells/fireball.hpp"


Spell_Fireball::Spell_Fireball(Vertagon* app) : Spell(app) {
    m_name = __func__;
}

WError Spell_Fireball::Load() {
    return WError(W_SUCCEEDED);
}

bool Spell_Fireball::Update(float fDeltaTime) {
    return false;
}

void Spell_Fireball::Cleanup() {
}
