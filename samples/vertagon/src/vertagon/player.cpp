#include "vertagon/player.hpp"
#include "vertagon/map/map.hpp"

#include <Wasabi/Physics/Bullet/WBulletRigidBody.hpp>

Player::Player(Vertagon* app): m_app(app) {
    m_cursor = nullptr;
    m_rigidBody = nullptr;

    m_yaw = 0.0f;
    m_pitch = 10.0f;
    m_recoilSpeed = 0.0f;
    m_recoilAcceleration = 0.0f;
    m_alreadyShot = false;

    m_controls.moveSpeed = 600000.0f;
    m_controls.jumpPower = 210000.0f;
    m_controls.mouseSensitivity = 0.1f;
    m_controls.maxPitchUp = 70.0f;
    m_controls.maxPitchDown = 50.0f;

    m_controls.bindings.moveForward = W_KEY_W;
    m_controls.bindings.strafeLeft = W_KEY_A;
    m_controls.bindings.moveBackward = W_KEY_S;
    m_controls.bindings.strafeRight = W_KEY_D;
    m_controls.bindings.jump = W_KEY_SPACE;

    memset(&m_controls.currentInput, 0, sizeof(m_controls.currentInput));
}

void Player::UpdateInput(float fDeltaTime) {
    /**
     * Shooting
     */
    if (m_controls.currentInput.isShootTriggered) {
        if (!m_alreadyShot) {
            m_alreadyShot = true;
            FireBullet();
        }
    }

    /**
     * Mouse movement
     */
    WVector2 delta = m_controls.currentInput.mousePos - m_controls.mouseReferencePosition;
    m_controls.mouseReferencePosition = m_controls.currentInput.mousePos;
    if (WVec2LengthSq(delta) > 25000.0f)
        delta = WVector2(0.0f, 0.0f);
    m_yaw += delta.x * m_controls.mouseSensitivity;
    m_pitch = std::max(-m_controls.maxPitchUp, std::min(m_controls.maxPitchDown, m_pitch + delta.y * m_controls.mouseSensitivity));

    /**
     * Recoil
     */
    m_pitch = std::max(-m_controls.maxPitchUp, std::min(m_controls.maxPitchDown, m_pitch - m_recoilSpeed * fDeltaTime));
    m_recoilSpeed += m_recoilAcceleration * fDeltaTime;
    m_recoilAcceleration = std::min(std::max(m_recoilAcceleration - m_recoilSpeed * 16.0f, -m_recoilSpeed * 32.0f), m_recoilSpeed * 32.0f);

    /**
     * Keyboard movement
     */
    WVector3 position = m_rigidBody->GetPosition();
    float rad = 0.3f;
    bool isGrounded =
        m_app->PhysicsComponent->RayCast(position + WVector3(0.0f, -0.9f, 0.0f), position + WVector3(0.0f, -1.3f, 0.0f)) ||
        m_app->PhysicsComponent->RayCast(position + WVector3(-rad, -0.85f, -rad), position + WVector3(-rad, -1.3f, -rad)) ||
        m_app->PhysicsComponent->RayCast(position + WVector3(+rad, -0.85f, -rad), position + WVector3(+rad, -1.3f, -rad)) ||
        m_app->PhysicsComponent->RayCast(position + WVector3(-rad, -0.85f, +rad), position + WVector3(-rad, -1.3f, +rad)) ||
        m_app->PhysicsComponent->RayCast(position + WVector3(+rad, -0.85f, +rad), position + WVector3(+rad, -1.3f, +rad));

    WVector3 direction = WVector3();
    WVector3 lookVec = WVec3TransformNormal(WVector3(0.0f, 0.0f, 1.0f), WRotationMatrixY(W_DEGTORAD(m_yaw)));
    WVector3 rightVec = WVec3TransformNormal(WVector3(1.0f, 0.0f, 0.0f), WRotationMatrixY(W_DEGTORAD(m_yaw)));
    if (m_controls.currentInput.isForwardPressed)
        direction += lookVec;
    if (m_controls.currentInput.isBackwardPressed)
        direction -= lookVec;
    if (m_controls.currentInput.isStrafeLeftPressed)
        direction -= rightVec;
    if (m_controls.currentInput.isStrafeRightPressed)
        direction += rightVec;
    if (isGrounded) {
        float dirLen = WVec3Length(direction);
        WVector3 velocity = m_rigidBody->getLinearVelocity();
        if (dirLen > 0.01f) {
            direction /= dirLen;
            float speed = WVec3Length(velocity);
            float multiplier = std::max(std::min((27.0f - speed) / 20.0f, 2.0f), 0.01f);
            m_rigidBody->ApplyForce(direction * m_controls.moveSpeed * multiplier * fDeltaTime);
        }

        if (m_controls.currentInput.isJumpPressed && !m_controls.currentInput.isJumpTriggered) {
            m_controls.currentInput.isJumpTriggered = true;
            m_rigidBody->ApplyForce((WVector3(0.0f, 1.0f, 0.0f) + (direction * 0.6f)) * m_controls.jumpPower);
        }
    }
    m_rigidBody->SetAngle(WQuaternion());
}

void Player::UpdateCamera(float fDeltaTime) {
    WVector3 position = m_rigidBody->GetPosition();

    m_cam->SetPosition(position + WVector3(0.0f, 1.0f, 0.0f));
    m_cam->SetAngle(WQuaternion());
    m_cam->Yaw(m_yaw);
    m_cam->Pitch(m_pitch);

    m_flashLight->SetPosition(position);
    m_flashLight->SetAngle(WQuaternion());
    m_flashLight->Yaw(m_yaw);
    m_flashLight->Pitch(m_pitch);
}

void Player::FireBullet() {
    WVector2 target = m_cursor->GetPosition() + m_cursor->GetSize() / 2.0f;
    m_app->FireBullet(target);
    m_recoilSpeed += 100.0f;
}

WError Player::Load() {
    Cleanup();

    m_cam = m_app->CameraManager->GetDefaultCamera();

    m_flashLight = new WSpotLight(m_app);
    m_flashLight->SetIntensity(2.0f);
    m_flashLight->SetRange(100.0f);

    WImage* cursorImg = m_app->ImageManager->CreateImage();
    if (!cursorImg) return WError(W_ERRORUNK);

    /**
     * Create a crossfire image (+ sign)
     */
    uint32_t w = 20, h = 20, c = 4;
    uint8_t* cursorImgData = new uint8_t[w * h * c];
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            cursorImgData[(y*h+x)*c + 0] = 255; // r
            cursorImgData[(y*h+x)*c + 1] = 255; // g
            cursorImgData[(y*h+x)*c + 2] = 255; // b
            if (x == w / 2 || x == w / 2 - 1 || y == h / 2 || y == h / 2 - 1)
                cursorImgData[(y*h+x)*c + 3] = 155; // a
            else
                cursorImgData[(y*h+x)*c + 3] = 0; // a
        }
    }
    WError status = cursorImg->CreateFromPixelsArray((void*)cursorImgData, w, h, VK_FORMAT_R8G8B8A8_UNORM);
    delete[] cursorImgData;

    m_cursor = m_app->SpriteManager->CreateSprite(cursorImg);
    W_SAFE_REMOVEREF(cursorImg);

    if (!status || !m_cursor) return status;

    m_rigidBody = new WBulletRigidBody(m_app);
    status = m_rigidBody->Create(W_RIGID_BODY_CREATE_INFO::ForCapsule(1.0f, 0.5f, 50.0f, nullptr, WVector3(0.0f, 2.0f, 0.0f)));
    if (!status) return status;
    m_rigidBody->SetLinearDamping(0.7f);
    m_rigidBody->SetAngularDamping(1.0f);
    m_rigidBody->SetBouncingPower(0.0f);
    m_rigidBody->SetFriction(4.0f);

    m_controls.mouseReferencePosition.x = m_app->WindowAndInputComponent->MouseX();
    m_controls.mouseReferencePosition.y = m_app->WindowAndInputComponent->MouseY();
    m_app->WindowAndInputComponent->SetCursorMotionMode(true);

    m_rigidBody->SetPosition(m_app->m_map->GetSpawnPoint());

    return WError(W_SUCCEEDED);
}

void Player::Update(float fDeltaTime) {
    Map* map = m_app->m_map;
    if (m_rigidBody->GetPosition().y < map->GetMinPoint())
        map->RandomSpawn(m_rigidBody);

    UpdateInput(fDeltaTime);
    UpdateCamera(fDeltaTime);

    float WW = (float)m_app->WindowAndInputComponent->GetWindowWidth();
    float WH = (float)m_app->WindowAndInputComponent->GetWindowHeight();
    m_cursor->SetPosition((WVector2(WW, WH) - m_cursor->GetSize()) / 2.0f);
}

void Player::Cleanup() {
    W_SAFE_REMOVEREF(m_cursor);
    W_SAFE_REMOVEREF(m_rigidBody);
}

void Player::OnMouseDown(W_MOUSEBUTTON button, double mx, double my) {
    UNREFERENCED_PARAMETER(mx);
    UNREFERENCED_PARAMETER(my);
    if (button == MOUSE_LEFT)
        m_controls.currentInput.isShootTriggered = true;
}

void Player::OnMouseUp(W_MOUSEBUTTON button, double mx, double my) {
    if (button == MOUSE_LEFT) {
        m_controls.currentInput.isShootTriggered = false;
        m_alreadyShot = false;
    }
}

void Player::OnMouseMove(double mx, double my) {
    m_controls.currentInput.mousePos = WVector2((float)mx, (float)my);
}

void Player::OnKeyDown(uint32_t c) {
    if (c == m_controls.bindings.moveForward)
        m_controls.currentInput.isForwardPressed = true;
    if (c == m_controls.bindings.moveBackward)
        m_controls.currentInput.isBackwardPressed = true;
    if (c == m_controls.bindings.strafeLeft)
        m_controls.currentInput.isStrafeLeftPressed = true;
    if (c == m_controls.bindings.strafeRight)
        m_controls.currentInput.isStrafeRightPressed = true;
    if (c == m_controls.bindings.jump)
        m_controls.currentInput.isJumpPressed = true;
}

void Player::OnKeyUp(uint32_t c) {
    if (c == m_controls.bindings.moveForward)
        m_controls.currentInput.isForwardPressed = false;
    if (c == m_controls.bindings.moveBackward)
        m_controls.currentInput.isBackwardPressed = false;
    if (c == m_controls.bindings.strafeLeft)
        m_controls.currentInput.isStrafeLeftPressed = false;
    if (c == m_controls.bindings.strafeRight)
        m_controls.currentInput.isStrafeRightPressed = false;
    if (c == m_controls.bindings.jump) {
        m_controls.currentInput.isJumpPressed = false;
        m_controls.currentInput.isJumpTriggered = false;
    }
}

void Player::OnInput(uint32_t c) {
    UNREFERENCED_PARAMETER(c);
}

WVector3 Player::GetPosition() const {
    return m_rigidBody->GetPosition();
}
