#include "vertagon/game.hpp"
#include "vertagon/player.hpp"
#include "vertagon/map.hpp"
#include "vertagon/enemies.hpp"

#include <Wasabi/Renderers/ForwardRenderer/WForwardRenderer.hpp>
#include <Wasabi/Renderers/DeferredRenderer/WDeferredRenderer.hpp>
#include <Wasabi/Physics/Bullet/WBulletPhysics.hpp>

Vertagon::GameState::GameState(Wasabi* app): WGameState(app) {
}

void Vertagon::GameState::OnMouseDown(W_MOUSEBUTTON button, double mx, double my) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnMouseDown(button, mx, my);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(m_app->Timer.GetElapsedTime(), IT_MOUSEDOWN, button, mx, my));
}

void Vertagon::GameState::OnMouseUp(W_MOUSEBUTTON button, double mx, double my) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnMouseUp(button, mx, my);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(m_app->Timer.GetElapsedTime(), IT_MOUSEUP, button, mx, my));
}

void Vertagon::GameState::OnMouseMove(double mx, double my) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnMouseMove(mx, my);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(m_app->Timer.GetElapsedTime(), IT_MOUSEMOVE, mx, my));
}

void Vertagon::GameState::OnKeyDown(char c) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnKeyDown(c);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(m_app->Timer.GetElapsedTime(), IT_KEYDOWN, c));
}

void Vertagon::GameState::OnKeyUp(char c) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnKeyUp(c);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(m_app->Timer.GetElapsedTime(), IT_KEYUP, c));
}

void Vertagon::GameState::OnInput(char c) {
    // pass on the input to the player class
    // ((Vertagon*)m_app)->m_player->OnInput(c);
    ((Vertagon*)m_app)->m_laggedInput.push_back(
        INPUT_DATA(m_app->Timer.GetElapsedTime(), IT_INPUT, c));
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
                    m_player->OnKeyDown(m_laggedInput[0].c);
                    break;
                case IT_KEYUP:
                    m_player->OnKeyUp(m_laggedInput[0].c);
                    break;
                case IT_INPUT:
                    m_player->OnInput(m_laggedInput[0].c);
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
}

WError Vertagon::Setup() {
    // start the engine
    WError status = StartEngine(640, 480);
    if (!status) {
        WindowAndInputComponent->ShowErrorMessage(status.AsString());
        return status;
    }

	PhysicsComponent->Start();
    PhysicsComponent->SetGravity(WVector3(0.0f, -100.0f, 0.0f));

    status = m_map->Load();
    if (!status) {
        WindowAndInputComponent->ShowErrorMessage(status.AsString());
        return status;
    }

    status = m_enemySystem->Load();
    if (!status) {
        WindowAndInputComponent->ShowErrorMessage(status.AsString());
        return status;
    }

    status = m_player->Load();
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
    return true; // return true to continue to next frame
}

void Vertagon::Cleanup() {
    m_map->Cleanup();
    m_enemySystem->Cleanup();
    m_player->Cleanup();

    delete m_map;
    delete m_enemySystem;
    delete m_player;
}

WError Vertagon::SetupRenderer() {
    // return WInitializeForwardRenderer(this);
    return WInitializeDeferredRenderer(this);
}

WPhysicsComponent* Vertagon::CreatePhysicsComponent() {
	WBulletPhysics* physics = new WBulletPhysics(this);
	SetEngineParam<int>("maxBulletDebugLines", 10000);
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

Wasabi* WInitialize() {
    return new Vertagon();
}
