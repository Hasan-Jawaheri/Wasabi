#pragma once

#include "vertagon/common.hpp"

class Vertagon : public Wasabi {
    friend class GameState;

    class GameState : public WGameState {
    public:
        GameState(Wasabi* app);

        virtual void OnMouseDown(W_MOUSEBUTTON button, double mx, double my) override;
        virtual void OnMouseUp(W_MOUSEBUTTON button, double mx, double my) override;
        virtual void OnMouseMove(double mx, double my) override;
        virtual void OnKeyDown(uint32_t c) override;
        virtual void OnKeyUp(uint32_t c) override;
        virtual void OnInput(uint32_t c) override;
    };

    enum INPUT_TYPE {
        IT_MOUSEDOWN,
        IT_MOUSEUP,
        IT_MOUSEMOVE,
        IT_KEYDOWN,
        IT_KEYUP,
        IT_INPUT,
    };
    struct INPUT_DATA {
        float timestamp;
        INPUT_TYPE type;
        W_MOUSEBUTTON btn;
        double mx, my;
        uint32_t key;

        INPUT_DATA(float _time, INPUT_TYPE _type, W_MOUSEBUTTON _button, double x, double y) {
            timestamp = _time;
            type = _type;
            btn = _button;
            mx = x;
            my = y;
        }

        INPUT_DATA(float _time, INPUT_TYPE _type, double x, double y) {
            timestamp = _time;
            type = _type;
            mx = x;
            my = y;
        }

        INPUT_DATA(float _time, INPUT_TYPE _type, uint32_t _key) {
            timestamp = _time;
            type = _type;
            key = _key;
        }
    };

    std::vector<INPUT_DATA> m_laggedInput;
    void DispatchLaggedInput(float fDeltaTime);

public:
    Vertagon();

    class Map* m_map;
    class EnemySystem* m_enemySystem;
    class Player* m_player;

    virtual WError Setup() override;
    virtual bool Loop(float fDeltaTime) override;
    virtual void Cleanup() override;

    virtual WError SetupRenderer() override;
    virtual WPhysicsComponent* CreatePhysicsComponent() override;

    void FireBullet(WVector2 target);
    WError UnsmoothFeometryNormals(WGeometry* geometry);
};
