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

    m_towerParams.numPlatforms = 20; // number of platforms to create
    m_towerParams.platformLength = 80.0f; // the length of the 2D (top-down view) arc representing each platform
    m_towerParams.distanceBetweenPlatforms = 2.0f; // distance (arc) to leave between platforms
    m_towerParams.platformHeight = 2.0f; // height from the beginning of the platform to the end
    m_towerParams.heightBetweenPlatforms = 2.0f; // height difference between the end of a platform and beginning of the next one
    m_towerParams.towerRadius = 70.0f; // radius of the tower
    m_towerParams.anglePerPlatform = W_RADTODEG(m_towerParams.platformLength / m_towerParams.towerRadius); // angle occupied by each platform
    m_towerParams.anglePerGap = W_RADTODEG(m_towerParams.distanceBetweenPlatforms / m_towerParams.towerRadius); // angle occupied the gap between platforms
    m_towerParams.platformWidth = 20.0f;
    m_towerParams.platformResWidth = 4;
    m_towerParams.platformResLength = 16;
    m_towerParams.xzRandomness = 2.0f;
    m_towerParams.yRandomness = 0.4f;
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
    status = m_skyGeometry->CreateSphere(-5000.0f, 28, 28);
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

    return BuildTower();
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

    for (auto platform : m_tower) {
        W_SAFE_REMOVEREF(platform.geometry);
        W_SAFE_REMOVEREF(platform.rigidBody);
        W_SAFE_REMOVEREF(platform.object);
    }
}


WError Map::BuildTower() {
    WError status = WError(W_SUCCEEDED);
    float curAngle = 0.0f;
    float curHeight = 0.0f;
    for (uint32_t platform = 0; platform < m_towerParams.numPlatforms && status; platform++) {
        status = BuildTowerPlatform(curAngle, curAngle + m_towerParams.anglePerPlatform, curHeight, curHeight + m_towerParams.platformHeight);
        curAngle += m_towerParams.anglePerPlatform + m_towerParams.anglePerGap;
        curHeight += m_towerParams.platformHeight + m_towerParams.heightBetweenPlatforms;
    }

    return status;
}

WError Map::BuildTowerPlatform(float angleFrom, float angleTo, float heightFrom, float heightTo) {
    float midAngle = (angleTo + angleFrom) / 2.0f;
    WVector3 center =
        (WVector3(cosf(W_DEGTORAD(midAngle)), 0.0f, sinf(W_DEGTORAD(midAngle))) * m_towerParams.towerRadius) +
        WVector3(0.0f, (heightTo + heightFrom) / 2.0f, 0.0f);

    WError status = WError(W_SUCCEEDED);
    TOWER_PLATFORM p;
    p.object = m_app->ObjectManager->CreateObject();
    p.geometry = new WGeometry(m_app);
    p.rigidBody = new WBulletRigidBody(m_app);

    if (!p.object) status = WError(W_ERRORUNK);

    if (status) {
        status = BuildPlatformGeometry(p.geometry, center, angleFrom, angleTo, heightFrom, heightTo);
        if (status) {
            status = p.object->SetGeometry(p.geometry);
            if (status) {
                p.object->SetName("Platform-" + std::to_string(heightFrom) + "-" + std::to_string(heightTo));

                p.object->GetMaterials().SetVariable("color", WColor(0.2f, 0.2f, 0.2f));
                p.object->GetMaterials().SetVariable("isTextured", 0);

                status = p.rigidBody->Create(W_RIGID_BODY_CREATE_INFO::ForComplexGeometry(p.geometry, true, nullptr, center));
                if (status) {
                    p.rigidBody->BindObject(p.object, p.object);
                }
            }
        }
    }

    if (!status) {
        W_SAFE_REMOVEREF(p.geometry);
        W_SAFE_REMOVEREF(p.object);
        W_SAFE_REMOVEREF(p.rigidBody);
    } else {
        m_tower.push_back(p);
    }

    return status;
}

WError Map::BuildPlatformGeometry(WGeometry* geometry, WVector3 center, float angleFrom, float angleTo, float heightFrom, float heightTo) {
    uint32_t xsegs = m_towerParams.platformResWidth;
    uint32_t zsegs = m_towerParams.platformResLength;
    float width = m_towerParams.platformWidth;

	uint32_t numVertices = (xsegs+1) * (zsegs+1) * 2 * 3;
	uint32_t numIndices = numVertices;

	//allocate the plain vertices
	vector<WDefaultVertex> vertices(numVertices);
	vector<uint32_t> indices(numIndices);

    auto computeNormal = [&vertices](uint32_t v1, uint32_t v2, uint32_t v3) {
        WVector3 p1 = vertices[v1].pos;
        WVector3 p2 = vertices[v2].pos;
        WVector3 p3 = vertices[v3].pos;
        WVector3 U = p2 - p1;
        WVector3 V = p3 - p1;
        WVector3 norm = WVec3Normalize(WVector3(U.y*V.z - U.z*V.y, U.z*V.x - U.x*V.z, U.x*V.y - U.y*V.x));
        vertices[v1].norm = vertices[v2].norm = vertices[v3].norm = norm;
    };

    std::vector<WVector3> randomValues((xsegs+2) * (zsegs+2));
    for (uint32_t i = 0; i < randomValues.size(); i++)
        randomValues[i] =
            WVector3(-m_towerParams.xzRandomness/2.0f, -m_towerParams.yRandomness/2.0f, -m_towerParams.xzRandomness/2.0f) +
            WVector3(m_towerParams.xzRandomness, m_towerParams.yRandomness, m_towerParams.xzRandomness) *
            WVector3(rand() % 10000, rand() % 10000, rand() % 10000) / 10000.0f;

	uint32_t curVert = 0;
    for (uint32_t celll = 0; celll < zsegs + 1; celll++) {
        float _v1 = ((float)celll / (float)(zsegs+1));
        float _v2 = ((float)(celll+1) / (float)(zsegs+1));
        for (uint32_t cellw = 0; cellw < xsegs + 1; cellw++) {
            float _u1 = (float)(cellw) / (float)(xsegs+1);
            float _u2 = (float)(cellw+1) / (float)(xsegs+1);

            auto vtx = [this, &center, &angleFrom, &angleTo, &width, &heightFrom, &heightTo](float u, float v, WVector3 randomness) {
                auto xc = [this, &angleFrom, &angleTo, &width](float u, float v) {
                    return cosf(W_DEGTORAD(v * (angleTo - angleFrom) + angleFrom)) * (m_towerParams.towerRadius - width / 2.0f + width * u);
                };
                auto yc = [&heightFrom, &heightTo](float v) {
                    return heightFrom + (heightTo - heightFrom) * v;
                };
                auto zc = [this, &angleFrom, &angleTo, &width](float u, float v) {
                    return sinf(W_DEGTORAD(v * (angleTo - angleFrom) + angleFrom)) * (m_towerParams.towerRadius - width / 2.0f + width * u);
                };

                WDefaultVertex vtx = WDefaultVertex(xc(u, v), yc(v), zc(u, v), 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, u, v);
                vtx.pos += randomness - center;
                return vtx;
            };

            auto randomness = [&randomValues](uint32_t l, uint32_t w, uint32_t W) {
                return randomValues[w*W + l];
            };

			vertices[curVert+0] = vtx(_u1, _v1, randomness(celll+0, cellw+0, xsegs+1));
			vertices[curVert+1] = vtx(_u1, _v2, randomness(celll+1, cellw+0, xsegs+1));
			vertices[curVert+2] = vtx(_u2, _v2, randomness(celll+1, cellw+1, xsegs+1));

			vertices[curVert+3] = vtx(_u1, _v1, randomness(celll+0, cellw+0, xsegs+1));
			vertices[curVert+4] = vtx(_u2, _v2, randomness(celll+1, cellw+1, xsegs+1));
			vertices[curVert+5] = vtx(_u2, _v1, randomness(celll+0, cellw+1, xsegs+1));

            for (uint32_t i = 0; i < 6; i++)
                indices[curVert + i] = curVert + i;
            computeNormal(curVert+0, curVert+1, curVert+2);
            computeNormal(curVert+3, curVert+4, curVert+5);
            curVert += 6;
        }
    }

    return geometry->CreateFromDefaultVerticesData(vertices, indices);
}
