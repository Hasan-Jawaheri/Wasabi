#include "vertagon/map.hpp"
#include "vertagon/game.hpp"
#include "vertagon/player.hpp"

#include <Wasabi/Renderers/DeferredRenderer/WGBufferRenderStage.hpp>
#include <Wasabi/Renderers/DeferredRenderer/WSceneCompositionRenderStage.hpp>
#include <Wasabi/Physics/Bullet/WBulletRigidBody.hpp>

class SkyVS : public WShader {
public:
	SkyVS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = {W_BOUND_RESOURCE(
			W_TYPE_UBO, 0, "uboPerObject", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "wvp"),
			}
		)};
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
			W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 1), // texture index
		}) };
		vector<uint8_t> code{
			#include "shaders/sky.vert.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

class SkyPS : public WShader {
public:
	SkyPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {};
		vector<uint8_t> code{
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
	WSceneCompositionRenderStage* SceneCompositionRenderStage = (WSceneCompositionRenderStage*)m_app->Renderer->GetRenderStage("WSceneCompositionRenderStage");

    /**
     * Liughting
     */
    // m_app->LightManager->GetDefaultLight()->Hide();
	SceneCompositionRenderStage->SetAmbientLight(WColor(0.01f, 0.01f, 0.01f));

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
    status = m_skyGeometry->CreateSphere(-5000.0f, 20, 20);
    if (!status) return status;
	status = ((Vertagon*)m_app)->UnsmoothFeometryNormals(m_skyGeometry);
	if (!status) return status;

    SkyPS* skyPS = new SkyPS(m_app);
    skyPS->Load();
	SkyVS* skyVS = new SkyVS(m_app);
	skyVS->Load();

	if (!skyPS->Valid() || !skyVS->Valid())
		status = WError(W_NOTVALID);

	if (status) {
		m_skyEffect = new WEffect(m_app);
		m_skyEffect->SetRenderFlags(EFFECT_RENDER_FLAG_RENDER_FORWARD | EFFECT_RENDER_FLAG_TRANSLUCENT);

		status = m_skyEffect->BindShader(skyVS);
		if (status) {
			status = m_skyEffect->BindShader(skyPS);
			if (status) {
				status = m_skyEffect->BuildPipeline(SceneCompositionRenderStage->GetRenderTarget());
			}
		}
	}

	W_SAFE_REMOVEREF(skyPS);
	W_SAFE_REMOVEREF(skyVS);
    if (!status) return status;

    m_sky = m_app->ObjectManager->CreateObject(m_skyEffect, 0);
    if (!m_sky) return WError(W_ERRORUNK);
	m_sky->ClearEffects();
	m_sky->AddEffect(m_skyEffect, 0);
    status = m_sky->SetGeometry(m_skyGeometry);
    if (!status) return status;
    m_sky->SetName("Mapsky");

    m_sky->GetMaterials().SetVariable("color", WColor(0.9f, 0.2f, 0.2f));
    m_sky->GetMaterials().SetVariable("isTextured", 0);

    return WError(W_SUCCEEDED);
}

void Map::Update(float fDeltaTime) {
	WCamera* cam = m_app->CameraManager->GetDefaultCamera();
	m_sky->GetMaterials().SetVariable("wvp", WTranslationMatrix(((Vertagon*)m_app)->m_player->GetPosition()) * cam->GetViewMatrix() * cam->GetProjectionMatrix());
}

void Map::Cleanup() {
    W_SAFE_REMOVEREF(m_sky);
    W_SAFE_REMOVEREF(m_skyGeometry);
    W_SAFE_REMOVEREF(m_skyEffect);
    W_SAFE_REMOVEREF(m_plain);
    W_SAFE_REMOVEREF(m_plainGeometry);
    W_SAFE_REMOVEREF(m_rigidBody);
}
