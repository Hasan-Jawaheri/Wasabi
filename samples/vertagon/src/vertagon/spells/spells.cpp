#include "vertagon/spells/spells.hpp"


Spell::Spell(Vertagon* app) : m_app(app) {
}

WError Spell::Load() {
    return WError(W_SUCCEEDED);
}

bool Spell::Update(float fDeltaTime) {
    return false;
}

void Spell::Cleanup() {
}


SpellSystem::SpellSystem(Vertagon* app) : m_app(app) {
}

WError SpellSystem::Load() {
    return WError(W_SUCCEEDED);
}

void SpellSystem::Update(float fDeltaTime) {
    for (uint32_t i = 0; i < m_spells.size(); i++) {
        if (!m_spells[i]->Update(fDeltaTime)) {
            m_spells[i]->Cleanup();
            m_spells.erase(m_spells.begin() + i);
            i--;
        }
    }
}

void SpellSystem::Cleanup() {
    for (auto spell : m_spells)
        spell->Cleanup();
    m_spells.clear();
}

void SpellSystem::CastSpell(std::shared_ptr<Spell> spell) {
    m_spells.push_back(spell);
    WError status = spell->Load();
    if (!status)
        m_app->WindowAndInputComponent->ShowErrorMessage("Spell failed to load (" + spell->m_name + "): " + status.AsString());
}
