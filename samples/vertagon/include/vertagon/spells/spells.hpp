#pragma once

#include "vertagon/common.hpp"


class Spell {
    friend class SpellSystem;

protected:
    Vertagon* m_app;
    std::string m_name;

public:
    Spell(Vertagon* app);

    virtual WError Load();
    virtual void Update(float fDeltaTime);
    virtual void Cleanup();
    virtual bool IsAlive();
};

class SpellSystem {
    friend class Spell;

    Vertagon* m_app;

    std::vector<std::shared_ptr<Spell>> m_spells;

public:
    SpellSystem(Vertagon* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();

    void CastSpell(std::shared_ptr<Spell> spell);
};
