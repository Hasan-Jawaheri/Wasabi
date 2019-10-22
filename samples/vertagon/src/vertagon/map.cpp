#include "vertagon/map.hpp"

#include <Wasabi/Renderers/DeferredRenderer/WGBufferRenderStage.hpp>
#include <Wasabi/Renderers/DeferredRenderer/WSceneCompositionRenderStage.hpp>
#include <Wasabi/Physics/Bullet/WBulletRigidBody.hpp>

class SkyPS : public WShader {
public:
	SkyPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {};
		vector<uint8_t> code {
			#include "shaders/sky.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

Map::Map(Wasabi* app): m_app(app) {
    m_plainGeometry = nullptr;
    m_plain = nullptr;
    m_rigidBody = nullptr;
	m_sky = nullptr;
	m_skyGeometry = nullptr;
    m_skyEffect = nullptr;
}

WError Map::Load() {
    Cleanup();

    WGBufferRenderStage* GBufferRenderStage = (WGBufferRenderStage*)m_app->Renderer->GetRenderStage("WGBufferRenderStage");

    /**
     * Liughting
     */
    // m_app->LightManager->GetDefaultLight()->Hide();
    WSceneCompositionRenderStage* sceneComposition = (WSceneCompositionRenderStage*)m_app->Renderer->GetRenderStage("WSceneCompositionRenderStage");
    sceneComposition->SetAmbientLight(WColor(0.01f, 0.01f, 0.01f));

    /**
     * Ground
     */
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

    /**
     * Sky
     */
    m_skyGeometry = new WGeometry(m_app);
    status = m_skyGeometry->CreateSphere(-200.0f);
    if (!status) return status;

    SkyPS* skyPS = new SkyPS(m_app);
    skyPS->Load();
    if (!skyPS->Valid()) return WError(W_NOTVALID);
    m_skyEffect = new WEffect(m_app);
    m_skyEffect->SetRenderFlags(EFFECT_RENDER_FLAG_RENDER_GBUFFER | EFFECT_RENDER_FLAG_TRANSLUCENT);

    status = m_skyEffect->BindShader(GBufferRenderStage->GetDefaultVertexShader());
    if (status) {
        status = m_skyEffect->BindShader(skyPS);
        if (status) {
            status = m_skyEffect->BuildPipeline(GBufferRenderStage->GetRenderTarget());
        }
    }

    W_SAFE_REMOVEREF(skyPS);
    if (!status) return status;

    m_sky = m_app->ObjectManager->CreateObject(m_skyEffect, 0);
    if (!m_sky) return WError(W_ERRORUNK);
    status = m_sky->SetGeometry(m_skyGeometry);
    if (!status) return status;
    m_sky->SetName("Mapsky");

    m_sky->GetMaterials().SetVariable("color", WColor(0.9f, 0.2f, 0.2f));
    m_sky->GetMaterials().SetVariable("isTextured", 0);

    return WError(W_SUCCEEDED);
}

void Map::Update(float fDeltaTime) {
}

void Map::Cleanup() {
    W_SAFE_REMOVEREF(m_sky);
    W_SAFE_REMOVEREF(m_skyGeometry);
    W_SAFE_REMOVEREF(m_skyEffect);
    W_SAFE_REMOVEREF(m_plain);
    W_SAFE_REMOVEREF(m_plainGeometry);
    W_SAFE_REMOVEREF(m_rigidBody);
}
