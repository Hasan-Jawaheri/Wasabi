#include "vertagon/map/map.hpp"
#include "vertagon/map/sky.hpp"
#include "vertagon/map/platforms.hpp"

#include <Wasabi/Renderers/DeferredRenderer/WGBufferRenderStage.hpp>
#include <Wasabi/Renderers/DeferredRenderer/WSceneCompositionRenderStage.hpp>
#include <Wasabi/Physics/Bullet/WBulletRigidBody.hpp>


Map::Map(Vertagon* app): m_app(app), m_platforms(app), m_sky(app) {
}

WError Map::Load() {
    Cleanup();

    /**
     * Lighting
     */
    WGBufferRenderStage* GBufferRenderStage = (WGBufferRenderStage*)m_app->Renderer->GetRenderStage("WGBufferRenderStage");
    WRenderStage* ForwardRenderStage = m_app->Renderer->GetRenderStage("WSceneCompositionRenderStage");
    if (!ForwardRenderStage) {
        ForwardRenderStage = m_app->Renderer->GetRenderStage("WForwardRenderStage");
        ((WForwardRenderStage*)ForwardRenderStage)->SetAmbientLight(WColor(0.3f, 0.3f, 0.3f));
    } else {
        ((WSceneCompositionRenderStage*)ForwardRenderStage)->SetAmbientLight(WColor(0.3f, 0.3f, 0.3f));
    }
    m_app->LightManager->GetDefaultLight()->Hide();

    /**
     * Sky
     */
    WError status = m_sky.Load(ForwardRenderStage);
    if (!status) return status;

    /**
     * Platforms
     */
    status = m_platforms.Load();
    if (!status) return status;

    return status;
}

void Map::Update(float fDeltaTime) {
    m_sky.Update(fDeltaTime);
    m_platforms.Update(fDeltaTime);
}

void Map::Cleanup() {
    m_sky.Cleanup();
    m_platforms.Cleanup();
}

WVector3 Map::GetSpawnPoint() const {
    return m_platforms.GetSpawnPoint();
}

void Map::RandomSpawn(WOrientation* object) const {
    m_platforms.RandomSpawn(object);
}

float Map::GetMinPoint() const {
    return -200.0f;
}
