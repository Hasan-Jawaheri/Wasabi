#include "vertagon/map/water.hpp"


Water::Water(Vertagon* app) : m_app(app) {
    m_object = nullptr;
    m_geometry = nullptr;

    m_params.waterSize = 800.0f;
    m_params.waterRes = 40;
}

WError Water::Load() {
    m_geometry = new WGeometry(m_app);
    WError status = m_geometry->CreatePlain(m_params.waterSize, m_params.waterRes, m_params.waterRes, W_GEOMETRY_CREATE_DYNAMIC | W_GEOMETRY_CREATE_REWRITE_EVERY_FRAME);
    if (!status) return status;
    status = m_app->UnsmoothGeometryNormals(m_geometry, W_GEOMETRY_CREATE_VB_DYNAMIC | W_GEOMETRY_CREATE_VB_REWRITE_EVERY_FRAME);
    if (!status) return status;

    m_object = m_app->ObjectManager->CreateObject();
    if (!m_object) return WError(W_ERRORUNK);
    m_object->SetGeometry(m_geometry);
    m_object->GetMaterials().SetVariable("color", WColor(0.45f, 0.8f, 0.95f, 0.6f));
    m_object->GetMaterials().SetVariable("isTextured", 0);
    m_object->GetMaterials().SetVariable("specularPower", 2.0f);
    m_object->GetMaterials().SetVariable("specularIntensity", 1.5f);

    return WError(W_SUCCEEDED);
}

void Water::Update(float fDeltaTime) {
    /**
     * Make a wave effect
     */
    WDefaultVertex* vertices;
    uint32_t numVertices = m_geometry->GetNumVertices();
    float phase = m_app->Timer.GetElapsedTime();
    float frequency = 3.0f;
    float speed = 1.0f;
    float scale = 4.0f;

    auto computeNormal = [&vertices](uint32_t v1, uint32_t v2, uint32_t v3) {
        WVector3 p1 = vertices[v1].pos;
        WVector3 p2 = vertices[v2].pos;
        WVector3 p3 = vertices[v3].pos;
        WVector3 U = p2 - p1;
        WVector3 V = p3 - p1;
        WVector3 norm = WVec3Normalize(WVector3(U.y * V.z - U.z * V.y, U.z * V.x - U.x * V.z, U.x * V.y - U.y * V.x));
        vertices[v1].norm = vertices[v2].norm = vertices[v3].norm = norm;
    };

    m_geometry->MapVertexBuffer((void**)(&vertices), W_MAP_READ | W_MAP_WRITE);
    for (uint32_t i = 0; i < numVertices; i++) {
        vertices[i].pos.y = 0.0f;
        float distanceInv = std::max(0.0f, (m_params.waterSize - WVec3Length(vertices[i].pos)) / m_params.waterSize);
        vertices[i].pos.y =
            (std::sin((vertices[i].pos.x * frequency / distanceInv + phase) * speed) +
             std::cos((vertices[i].pos.z * frequency / distanceInv + phase) * speed)) * scale * distanceInv;
        if ((i+1) % 3 == 0)
            computeNormal(i-2, i-1, i);
    }
    m_geometry->UnmapVertexBuffer(false);
}

void Water::Cleanup() {
    W_SAFE_REMOVEREF(m_object);
    W_SAFE_REMOVEREF(m_geometry);
}
