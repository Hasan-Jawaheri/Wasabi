#pragma once

#include "vertagon/common.hpp"


class Sky {
    Vertagon* m_app;
    WGeometry* m_geometry;
    WEffect* m_effect;
    WObject* m_object;

public:
    Sky(Vertagon* app);

    WError Load(WRenderStage* renderStage);
    void Update(float fDeltaTime);
    void Cleanup();
};
