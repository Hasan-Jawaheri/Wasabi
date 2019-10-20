#pragma once

#include "vertagon/common.hpp"

class Enemy {
    friend class EnemySystem;

    Wasabi* m_app;
    WObject* m_object;
    WRigidBody* m_rigidBody;
    WPointLight* m_light;
    uint32_t m_id;

    struct {
        float explodePeriod;
    } m_properties;

    struct {
        float explodeTime;
    } m_state;

public:
    Enemy(Wasabi* app);

    WError Load(uint32_t id);
    bool Update(float fDeltaTime);
    void Destroy();

    bool IsAlive();
    void Explode();
};

class EnemySystem {
    friend class Enemy;

    Wasabi* m_app;

    std::vector<Enemy*> m_enemies;
    std::vector<uint32_t> m_freeEnemyIds;
    uint32_t m_maxEnemies;
    float m_lastRespawn;
    float m_respawnInterval;

    struct {
        WGeometry* enemyGeometry;
        WEffect* enemyEffect;
    } m_resources;

public:
    EnemySystem(Wasabi* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();

    uint32_t GetMinHitID() const;
    uint32_t GetMaxHitID() const;
    void OnBulletFired(WObject* hitObject, WVector3 hitPoint, WVector2 hitUV, uint32_t hitFace);
};
