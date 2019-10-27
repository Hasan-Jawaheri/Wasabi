#include "vertagon/spells/spells.hpp"


Spell::Spell(Vertagon* app) : m_app(app) {
}

WError Spell::Load() {
    return WError(W_SUCCEEDED);
}

void Spell::Update(float fDeltaTime) {
}

void Spell::Cleanup() {
}

bool Spell::IsAlive() {
    return false;
}


SpellSystem::SpellSystem(Vertagon* app) : m_app(app) {
}

WError SpellSystem::Load() {
    return WError(W_SUCCEEDED);
}

void SpellSystem::Update(float fDeltaTime) {
    for (uint32_t i = 0; i < m_spells.size(); i++) {
        m_spells[i]->Update(fDeltaTime);
        if (!m_spells[i]->IsAlive()) {
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
    WError status = spell->Load();
    if (!status) {
        spell->Cleanup();
        m_app->WindowAndInputComponent->ShowErrorMessage("Spell failed to load (" + spell->m_name + "): " + status.AsString());
    } else
        m_spells.push_back(spell);
}
