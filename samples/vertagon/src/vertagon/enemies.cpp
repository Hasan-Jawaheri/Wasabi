#include "vertagon/enemies.hpp"
#include "vertagon/game.hpp"
#include "vertagon/player.hpp"

#include <Wasabi/Renderers/WRenderStage.hpp>
#include <Wasabi/Physics/Bullet/WBulletRigidBody.hpp>

class EnemyVS : public WShader {
public:
	EnemyVS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
            W_BOUND_RESOURCE(W_TYPE_UBO, 0, "uboPerFrame", {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "explosionRange"),
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, "percentage"),
            }),
			W_BOUND_RESOURCE(W_TYPE_PUSH_CONSTANT, 0, "pcPerObject", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "world"),
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "view"),
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projection"),
			}),
		};
	}

	virtual void Load(bool bSaveData = false) override {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = { W_INPUT_LAYOUT({
            W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
            W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
            W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
            W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
            W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 1), // texture index
		})};
		vector<uint8_t> code {
			#include "shaders/enemy.vert.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

class EnemyPS : public WShader {
public:
	EnemyPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
            EnemyVS::GetBoundResources()[0],
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 1, "diffuseTexture"),
		};
		vector<uint8_t> code {
			#include "shaders/enemy.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

Enemy::Enemy(Wasabi* app) {
    m_app = app;
    m_object = nullptr;
    m_light = nullptr;
    m_rigidBody = nullptr;

    m_properties.explodePeriod = 1.0f;

    m_state.explodeTime = -1.0f;
}

WError Enemy::Load(uint32_t id) {
    m_id = id;

    WGeometry* geometry = ((Vertagon*)m_app)->m_enemySystem->m_resources.enemyGeometry;
    WEffect* effect = ((Vertagon*)m_app)->m_enemySystem->m_resources.enemyEffect;

    m_object = m_app->ObjectManager->CreateObject(effect, 0, m_id);
    if (!m_object) {
        W_SAFE_REMOVEREF(geometry);
        return WError(W_ERRORUNK);
    }
    WError status = m_object->SetGeometry(geometry);
    if (!status) return status;

    m_light = new WPointLight(m_app);
    m_light->SetRange(10.0f);
    m_light->SetIntensity(1.0f);
    m_light->SetColor(WColor(0.7f, 0.5f, 0.4f));

    m_object->GetMaterials().SetVariable("explosionRange", 100.0f);
    m_object->GetMaterials().SetVariable("percentage", 0.0f);

    m_rigidBody = new WBulletRigidBody(m_app);
    status = m_rigidBody->Create(W_RIGID_BODY_CREATE_INFO::ForSphere(1.0f, 0.0f));
    if (!status) return status;
    m_rigidBody->BindObject(m_object, m_object);

    return WError(W_SUCCEEDED);
}

bool Enemy::Update(float fDeltaTime) {
    m_light->SetPosition(m_object->GetPosition() + WVector3(0, 2, 0));

    if (m_state.explodeTime > 0.0f) {
        float timeSinceExplosion = m_app->Timer.GetElapsedTime() - m_state.explodeTime;
        float explosionPercentage = timeSinceExplosion / m_properties.explodePeriod;
        m_light->SetIntensity(std::max(0.0f, 1.0f - explosionPercentage));
        if (explosionPercentage <= 1.0f) {
            m_object->GetMaterials().SetVariable("percentage", explosionPercentage);
        } else {
            return false;
        }
    }

    WCamera* cam = m_app->CameraManager->GetDefaultCamera();
    m_object->GetMaterials().SetVariable("world", m_object->GetWorldMatrix());
    m_object->GetMaterials().SetVariable("view", cam->GetViewMatrix());
    m_object->GetMaterials().SetVariable("projection", cam->GetProjectionMatrix());
    return true;
}

void Enemy::Destroy() {
    W_SAFE_REMOVEREF(m_object);
    W_SAFE_REMOVEREF(m_light);
    W_SAFE_REMOVEREF(m_rigidBody);
}

bool Enemy::IsAlive() {
    return m_state.explodeTime <= 0.0f;
}

void Enemy::Explode() {
    m_object->SetID(0);
    m_state.explodeTime = m_app->Timer.GetElapsedTime();
    W_SAFE_REMOVEREF(m_rigidBody);
}

EnemySystem::EnemySystem(Wasabi* app): m_app(app) {
    m_respawnInterval = 1.0f;
    m_lastRespawn = 0.0f;
    m_maxEnemies = 30;

    m_freeEnemyIds.resize(m_maxEnemies);
    for (uint32_t i = 0; i < m_maxEnemies; i++)
        m_freeEnemyIds[i] = i + 1;

    memset(&m_resources, 0, sizeof(m_resources));
}

WError EnemySystem::Load() {
    Cleanup();

    m_resources.enemyGeometry = new WGeometry(m_app);
    WError status = m_resources.enemyGeometry->CreateSphere(1.0f, 14, 14);
    if (!status) return status;
	status = ((Vertagon*)m_app)->UnsmoothFeometryNormals(m_resources.enemyGeometry);
    if (!status) return status;

	EnemyVS* vs = new EnemyVS(m_app);
	vs->Load();
	EnemyPS* ps = new EnemyPS(m_app);
	ps->Load();

    if (!vs->Valid() || !ps->Valid())
        status = WError(W_INVALIDPARAM);

    if (status) {
        m_resources.enemyEffect = new WEffect(m_app);
        m_resources.enemyEffect->SetRenderFlags(EFFECT_RENDER_FLAG_RENDER_GBUFFER | EFFECT_RENDER_FLAG_TRANSLUCENT);
        VkPipelineRasterizationStateCreateInfo rasterizationState = {};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL; // Solid polygon mode
        rasterizationState.cullMode = VK_CULL_MODE_NONE; // No culling
        rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationState.lineWidth = 1.0f;
        m_resources.enemyEffect->SetRasterizationState(rasterizationState);

        status = m_resources.enemyEffect->BindShader(vs);
        if (status) {
            status = m_resources.enemyEffect->BindShader(ps);
            if (status) {
                status = m_resources.enemyEffect->BuildPipeline(m_app->Renderer->GetRenderStage("WGBufferRenderStage")->GetRenderTarget());
            }
        }
    }

    W_SAFE_REMOVEREF(vs);
    W_SAFE_REMOVEREF(ps);
    if (!status) {
        W_SAFE_REMOVEREF(m_resources.enemyEffect);
        return status;
    }

    return WError(W_SUCCEEDED);
}

void EnemySystem::Update(float fDeltaTime) {
    Player* player = ((Vertagon*)m_app)->m_player;

	/**
	 * Enemies spawn
	 */
	if (m_enemies.size() < m_maxEnemies && m_app->Timer.GetElapsedTime() - m_lastRespawn > m_respawnInterval) {
		m_lastRespawn = m_app->Timer.GetElapsedTime();
		// spawn new enemy
		Enemy* enemy = new Enemy(m_app);
		WError status = enemy->Load(m_freeEnemyIds[0]);
		if (!status) {
			enemy->Destroy();
			delete enemy;
		} else {
			m_freeEnemyIds.erase(m_freeEnemyIds.begin());
			m_enemies.push_back(enemy);
			float x = ((float)(std::rand() % 1000 - 500) / 500.0f) * 25.0f;
			float z = ((float)(std::rand() % 1000 - 500) / 500.0f) * 25.0f;
			enemy->m_rigidBody->SetPosition(WVector3(x, 0.0f, z) + player->GetPosition());
		}
	}

	/**
	 * Enemies death
	 */
    for (int32_t i = 0; i < m_enemies.size(); i++) {
        if (!m_enemies[i]->Update(fDeltaTime) || WVec3LengthSq(player->GetPosition() - m_enemies[i]->m_object->GetPosition()) > 100.0f * 100.0f) {
            m_freeEnemyIds.push_back(m_enemies[i]->m_id);
            m_enemies[i]->Destroy();
            delete m_enemies[i];
            m_enemies.erase(m_enemies.begin() + i);
            i--;
        }
    }
}

void EnemySystem::Cleanup() {
    for (auto enemy : m_enemies) {
        enemy->Destroy();
        delete enemy;
    }
    m_enemies.clear();

    W_SAFE_REMOVEREF(m_resources.enemyEffect);
    W_SAFE_REMOVEREF(m_resources.enemyGeometry);
}

uint32_t EnemySystem::GetMinHitID() const {
    return 1;
}

uint32_t EnemySystem::GetMaxHitID() const {
    return m_maxEnemies;
}

void EnemySystem::OnBulletFired(WObject* hitObject, WVector3 hitPoint, WVector2 hitUV, uint32_t hitFace) {
    for (auto enemy : m_enemies) {
        if (enemy->m_object == hitObject && enemy->IsAlive()) {
            enemy->Explode();
        }
    }
}
