#pragma once

#include "vertagon/common.hpp"

class Map {
    Wasabi* m_app;

    WGeometry* m_plainGeometry;
    WObject* m_plain;
    WRigidBody* m_rigidBody;

    WGeometry* m_skyGeometry;
    WEffect* m_skyEffect;
    WObject* m_sky;

public:
    Map(Wasabi* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();
};
