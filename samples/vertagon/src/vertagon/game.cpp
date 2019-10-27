#include "vertagon/game.hpp"
#include "vertagon/player.hpp"
#include "vertagon/map/map.hpp"
#include "vertagon/enemies.hpp"
#include "vertagon/spells/spells.hpp"

#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderer.hpp>
#include <Wasabi/Renderers/DeferredRenderer/WDeferredRenderer.hpp>
#include <Wasabi/Physics/Bullet/WBulletPhysics.hpp>

Vertagon::GameState::GameState(Wasabi* app): WGameState(app) {
}

void Vertagon::GameState::OnMouseDown(W_MOUSEBUTTON button, double mx, double my) {
    // pass on the input to the player class
    // m_app->m_player->OnMouseDown(button, mx, my);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(((Vertagon*)m_app)->Timer.GetElapsedTime(), IT_MOUSEDOWN, button, mx, my));
}

void Vertagon::GameState::OnMouseUp(W_MOUSEBUTTON button, double mx, double my) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnMouseUp(button, mx, my);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(((Vertagon*)m_app)->Timer.GetElapsedTime(), IT_MOUSEUP, button, mx, my));
}

void Vertagon::GameState::OnMouseMove(double mx, double my) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnMouseMove(mx, my);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(((Vertagon*)m_app)->Timer.GetElapsedTime(), IT_MOUSEMOVE, mx, my));
}

void Vertagon::GameState::OnKeyDown(uint32_t c) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnKeyDown(c);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(((Vertagon*)m_app)->Timer.GetElapsedTime(), IT_KEYDOWN, c));
}

void Vertagon::GameState::OnKeyUp(uint32_t c) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnKeyUp(c);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(((Vertagon*)m_app)->Timer.GetElapsedTime(), IT_KEYUP, c));
}

void Vertagon::GameState::OnInput(uint32_t c) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnInput(c);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(((Vertagon*)m_app)->Timer.GetElapsedTime(), IT_INPUT, c));
}

void Vertagon::DispatchLaggedInput(float fDeltaTime) {
    static float totalLagMS = 0.0f;

    if (WindowAndInputComponent->KeyDown(W_KEY_UP))
        totalLagMS =  totalLagMS + 50.0f * fDeltaTime;
    if (WindowAndInputComponent->KeyDown(W_KEY_DOWN))
        totalLagMS = std::max(0.0f, totalLagMS - 50.0f * fDeltaTime);
    TextComponent->RenderText("Total lag: " + std::to_string((int)totalLagMS) + "ms", 5, 40, 32);

    float curTime = Timer.GetElapsedTime();
    float totalLagSeconds = totalLagMS / 1000.0f;
    while (m_laggedInput.size() > 0) {
        if (m_laggedInput[0].timestamp + totalLagSeconds <= curTime) {
            switch (m_laggedInput[0].type) {
                case IT_MOUSEDOWN:
                    m_player->OnMouseDown(m_laggedInput[0].btn, m_laggedInput[0].mx, m_laggedInput[0].my);
                    break;
                case IT_MOUSEUP:
                    m_player->OnMouseUp(m_laggedInput[0].btn, m_laggedInput[0].mx, m_laggedInput[0].my);
                    break;
                case IT_MOUSEMOVE:
                    m_player->OnMouseMove(m_laggedInput[0].mx, m_laggedInput[0].my);
                    break;
                case IT_KEYDOWN:
                    m_player->OnKeyDown(m_laggedInput[0].key);
                    break;
                case IT_KEYUP:
                    m_player->OnKeyUp(m_laggedInput[0].key);
                    break;
                case IT_INPUT:
                    m_player->OnInput(m_laggedInput[0].key);
                    break;
            }
            m_laggedInput.erase(m_laggedInput.begin());
        } else
            break;
    }
}

Vertagon::Vertagon() {
    m_map = new Map(this);
    m_player = new Player(this);
    m_enemySystem = new EnemySystem(this);
    m_spellSystem = new SpellSystem(this);
}

WError Vertagon::Setup() {
    // SetEngineParam("enableVulkanValidation", false);
    // start the engine
    maxFPS = 0;
    WError status = StartEngine(640, 480);
    if (!status) {
        WindowAndInputComponent->ShowErrorMessage(status.AsString());
        return status;
    }

    PhysicsComponent->Start();
    PhysicsComponent->SetGravity(WVector3(0.0f, -100.0f, 0.0f));

    status = m_map->Load();
    if (status) {
        status = m_enemySystem->Load();
        if (status) {
            status = m_player->Load();
            if (status) {
                status = m_spellSystem->Load();
            }
        }
    }

    if (!status) {
        WindowAndInputComponent->ShowErrorMessage(status.AsString());
        return status;
    }

    SwitchState(new GameState(this));

    return status;
}

bool Vertagon::Loop(float fDeltaTime) {
    DispatchLaggedInput(fDeltaTime);

    TextComponent->RenderText("FPS: " + std::to_string(FPS), 5, 5, 32);
    m_map->Update(fDeltaTime);
    m_enemySystem->Update(fDeltaTime);
    m_player->Update(fDeltaTime);
    m_spellSystem->Update(fDeltaTime);
    return true; // return true to continue to next frame
}

void Vertagon::Cleanup() {
    m_map->Cleanup();
    m_enemySystem->Cleanup();
    m_player->Cleanup();
    m_spellSystem->Cleanup();

    W_SAFE_DELETE(m_map);
    W_SAFE_DELETE(m_enemySystem);
    W_SAFE_DELETE(m_player);
    W_SAFE_DELETE(m_spellSystem);
}

WError Vertagon::SetupRenderer() {
    return WInitializeForwardRenderer(this);
    // return WInitializeDeferredRenderer(this);
}

WPhysicsComponent* Vertagon::CreatePhysicsComponent() {
    WBulletPhysics* physics = new WBulletPhysics(this);
    SetEngineParam<int>("maxBulletDebugLines", 100000);
    WError werr = physics->Initialize(false);
    if (!werr)
        W_SAFE_DELETE(physics);
    return physics;
}

void Vertagon::FireBullet(WVector2 target) {
    WVector3 hitPoint;
    WVector2 hitUV;
    uint32_t hitFace;
    WObject* hitObject = ObjectManager->PickObject((double)target.x, (double)target.y, false, m_enemySystem->GetMinHitID(), m_enemySystem->GetMaxHitID(), &hitPoint, &hitUV, &hitFace);
    if (hitObject) {
        m_enemySystem->OnBulletFired(hitObject, hitPoint, hitUV, hitFace);
    }
}

WError Vertagon::UnsmoothGeometryNormals(WGeometry* geometry) {
    uint32_t* indices;
    uint32_t numIndices = geometry->GetNumIndices();
    uint32_t* newIndices = new uint32_t[numIndices];

    WDefaultVertex* vertices;
    uint32_t numVertices = numIndices;
    WDefaultVertex* newVertices = new WDefaultVertex[numVertices];

    WError status = geometry->MapIndexBuffer((void**)&indices, W_MAP_READ);
    if (status) {
        memcpy(newIndices, indices, sizeof(uint32_t) * numIndices);
        geometry->UnmapIndexBuffer();

        status = geometry->MapVertexBuffer((void**)&vertices, W_MAP_READ);

        if (status) {
            for (uint32_t face = 0; face < numIndices / 3; face++) {
                WDefaultVertex v1 = vertices[newIndices[face * 3 + 0]];
                WDefaultVertex v2 = vertices[newIndices[face * 3 + 1]];
                WDefaultVertex v3 = vertices[newIndices[face * 3 + 2]];
                WVector3 norm = WVec3Normalize((v1.pos + v2.pos + v3.pos) / 3.0f);
                newVertices[face * 3 + 0] = v1;
                newVertices[face * 3 + 0].norm = norm;
                newVertices[face * 3 + 1] = v2;
                newVertices[face * 3 + 1].norm = norm;
                newVertices[face * 3 + 2] = v3;
                newVertices[face * 3 + 2].norm = norm;
                newIndices[face * 3 + 0] = face * 3 + 0;
                newIndices[face * 3 + 1] = face * 3 + 1;
                newIndices[face * 3 + 2] = face * 3 + 2;
            }

            geometry->UnmapVertexBuffer(false);

            status = geometry->CreateFromData(newVertices, numVertices, newIndices, numIndices);
        }
    }

    W_SAFE_DELETE_ARRAY(newVertices);
    W_SAFE_DELETE_ARRAY(newIndices);
    return status;
}

Wasabi* WInitialize() {
    return new Vertagon();
}
