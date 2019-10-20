#include "vertagon/map.hpp"

#include <Wasabi/Renderers/DeferredRenderer/WSceneCompositionRenderStage.hpp>
#include <Wasabi/Physics/Bullet/WBulletRigidBody.hpp>

Map::Map(Wasabi* app): m_app(app) {
    m_plainGeometry = nullptr;
    m_plain = nullptr;
    m_rigidBody = nullptr;
	m_atmosphere = nullptr;
	m_atmosphereGeometry = nullptr;
}

WError Map::Load() {
    Cleanup();

    // m_app->LightManager->GetDefaultLight()->Hide();
    WSceneCompositionRenderStage* sceneComposition = (WSceneCompositionRenderStage*)m_app->Renderer->GetRenderStage("WSceneCompositionRenderStage");
    sceneComposition->SetAmbientLight(WColor(0.01f, 0.01f, 0.01f));

    m_plainGeometry = new WGeometry(m_app);
    WError status = m_plainGeometry->CreatePlain(100.0f, 0, 0);
    if (!status) return status;

    m_plain = m_app->ObjectManager->CreateObject();
    if (!m_plain) return WError(W_ERRORUNK);
    status = m_plain->SetGeometry(m_plainGeometry);
    if (!status) return status;
    m_plain->SetName("MapPlain");

    m_plain->GetMaterials().SetVariable("color", WColor(0.2f, 0.2f, 0.2f));
    m_plain->GetMaterials().SetVariable("isTextured", 0);

    m_rigidBody = new WBulletRigidBody(m_app);
    status = m_rigidBody->Create(W_RIGID_BODY_CREATE_INFO::ForCube(WVector3(100.0f, 0.01f, 100.0f), 0.0f));
    if (!status) return status;

    m_atmosphereGeometry = new WGeometry(m_app);
    status = m_atmosphereGeometry->CreateSphere(-200.0f);
    if (!status) return status;

    m_atmosphere = m_app->ObjectManager->CreateObject();
    if (!m_atmosphere) return WError(W_ERRORUNK);
    status = m_atmosphere->SetGeometry(m_atmosphereGeometry);
    if (!status) return status;
    m_atmosphere->SetName("MapAtmosphere");

    m_atmosphere->GetMaterials().SetVariable("color", WColor(0.9f, 0.2f, 0.2f));
    m_atmosphere->GetMaterials().SetVariable("isTextured", 0);

    return WError(W_SUCCEEDED);
}

void Map::Update(float fDeltaTime) {
}

void Map::Cleanup() {
    W_SAFE_REMOVEREF(m_atmosphere);
    W_SAFE_REMOVEREF(m_atmosphereGeometry);
    W_SAFE_REMOVEREF(m_plain);
    W_SAFE_REMOVEREF(m_plainGeometry);
    W_SAFE_REMOVEREF(m_rigidBody);
}
