#pragma once

#include "vertagon/common.hpp"


class Water {
    Vertagon* m_app;

    WGeometry* m_geometry;
    WObject* m_object;

    struct {
        float waterSize;
        uint32_t waterRes;
    } m_params;

public:
    Water(Vertagon* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();
};
