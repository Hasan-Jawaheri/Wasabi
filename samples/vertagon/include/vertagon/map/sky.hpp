#pragma once

#include "vertagon/common.hpp"


class Sky {
    Wasabi* m_app;
    WGeometry* m_geometry;
    WEffect* m_effect;
    WObject* m_object;

public:
    Sky(Wasabi* app);

    WError Load(WRenderStage* renderStage);
    void Update(float fDeltaTime);
    void Cleanup();
};
