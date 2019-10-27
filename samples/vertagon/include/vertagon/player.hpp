#pragma once

#include "vertagon/common.hpp"

class Player {
    Vertagon* m_app;
    WCamera* m_cam;

    WSprite* m_cursor;
    WRigidBody* m_rigidBody;

    float m_yaw;
    float m_pitch;
    bool m_alreadyShot;
    float m_recoilSpeed;
    float m_recoilAcceleration;

    struct {
        float moveSpeed;
        float jumpPower;
        float mouseSensitivity;
        float maxPitchUp;
        float maxPitchDown;
        WVector2 mouseReferencePosition;

        struct {
            char moveForward;
            char moveBackward;
            char strafeLeft;
            char strafeRight;
            char jump;
        } bindings;

        struct {
            WVector2 mousePos;
            bool isForwardPressed;
            bool isBackwardPressed;
            bool isStrafeLeftPressed;
            bool isStrafeRightPressed;
            bool isJumpPressed;
            bool isShootTriggered;
            bool isJumpTriggered;
        } currentInput;
    } m_controls;

    struct {
        std::shared_ptr<class Spell> fireball;
    } m_casting;

    void UpdateInput(float fDeltaTime);
    void UpdateCamera(float fDeltaTime);

public:
    Player(Vertagon* app);

    WError Load();
    void Update(float fDeltaTime);
    void Cleanup();

    void OnMouseDown(W_MOUSEBUTTON button, double mx, double my);
    void OnMouseUp(W_MOUSEBUTTON button, double mx, double my);
    void OnMouseMove(double mx, double my);
    void OnKeyDown(uint32_t key);
    void OnKeyUp(uint32_t key);
    void OnInput(uint32_t key);

    WVector3 GetPosition() const;
};
