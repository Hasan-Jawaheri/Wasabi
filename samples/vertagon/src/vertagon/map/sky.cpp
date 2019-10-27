#include "vertagon/map/sky.hpp"
#include "vertagon/player.hpp"
#include "vertagon/game.hpp"


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
            #include "../shaders/sky.vert.glsl.spv"
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
            #include "../shaders/sky.frag.glsl.spv"
        };
        LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
    }
};

Sky::Sky(Wasabi* app): m_app(app) {
    m_geometry = nullptr;
    m_effect = nullptr;
    m_object = nullptr;
}

WError Sky::Load(WRenderStage* renderStage) {
    Cleanup();

    m_geometry = new WGeometry(m_app);
    WError status = m_geometry->CreateSphere(-5000.0f, 28, 28);
    if (!status) return status;
    status = ((Vertagon*)m_app)->UnsmoothGeometryNormals(m_geometry);
    if (!status) return status;

    SkyPS* skyPS = new SkyPS(m_app);
    skyPS->Load();
    SkyVS* skyVS = new SkyVS(m_app);
    skyVS->Load();

    if (!skyPS->Valid() || !skyVS->Valid())
        status = WError(W_NOTVALID);

    if (status) {
        m_effect = new WEffect(m_app);
        m_effect->SetRenderFlags(EFFECT_RENDER_FLAG_RENDER_FORWARD | EFFECT_RENDER_FLAG_TRANSLUCENT);

        status = m_effect->BindShader(skyVS);
        if (status) {
            status = m_effect->BindShader(skyPS);
            if (status) {
                status = m_effect->BuildPipeline(renderStage->GetRenderTarget());
            }
        }
    }

    W_SAFE_REMOVEREF(skyPS);
    W_SAFE_REMOVEREF(skyVS);
    if (!status) return status;

    m_object = m_app->ObjectManager->CreateObject(m_effect, 0);
    if (!m_object) return WError(W_ERRORUNK);
    m_object->ClearEffects();
    m_object->AddEffect(m_effect, 0);
    status = m_object->SetGeometry(m_geometry);
    if (!status) return status;
    m_object->SetName("Mapsky");

    m_object->GetMaterials().SetVariable("color", WColor(0.9f, 0.2f, 0.2f));
    m_object->GetMaterials().SetVariable("isTextured", 0);

    return status;
}

void Sky::Update(float fDeltaTime) {
    WCamera* cam = m_app->CameraManager->GetDefaultCamera();
    m_object->GetMaterials().SetVariable("wvp", WTranslationMatrix(((Vertagon*)m_app)->m_player->GetPosition()) * cam->GetViewMatrix() * cam->GetProjectionMatrix());
}

void Sky::Cleanup() {
    W_SAFE_REMOVEREF(m_object);
    W_SAFE_REMOVEREF(m_geometry);
    W_SAFE_REMOVEREF(m_effect);
}
