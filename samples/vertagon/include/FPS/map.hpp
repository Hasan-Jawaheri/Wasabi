#pragma once

#include "FPS/common.hpp"

class Map {
    Wasabi* m_app;

    WGeometry* m_plainGeometry;
    WObject* m_plain;
    WRigidBody* m_rigidBody;

    WGeometry* m_atmosphereGeometry;
    WObject* m_atmosphere;

public:
    Map(Wasabi* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();
};
