#include <Windows.h>
#include "Wasabi.h"
#include "WTimer.h"
#include "WCore.h"
#include "WSound.h"

#ifdef _WIN32
#include "WWC_Win32.h"
#elif defined(__linux__)
#include "WWC_Linux.h"
#endif

/****************************************************************/
/*
 *
 * DUMMY USER CODE
 *
 */

class Kofta : public Wasabi {
public:
	WError Setup() {
		WError err = core->Init(500, 500);
		return err;
	}
	bool Loop(float fDeltaTime) {
		return true;
	}
	void Cleanup() {

	}
};

Wasabi* WInitialize() {
	return new Kofta();
}

/****************************************************************/

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow) {
	WInitializeTimers();

	WCore* core = new WCore();
	core->SoundComponent = new WSoundComponent();

#ifdef _WIN32
	core->WindowComponent = new WWC_Win32(core);
#elif defined(__linux__)
	core->WindowComponent = new WWC_Linux(core);
#endif

	Wasabi* app = WInitialize();
	app->maxFPS = 60.0f;
	app->core = core;

	if (app->Setup()) {
		WTimer tmr(W_TIMER_SECONDS);
		WTimer fpsChangeTmr(W_TIMER_SECONDS);
		fpsChangeTmr.Start();
		UINT numFrames = 0;
		float deltaTime = 0.0f;
		while (!app->core->__EXIT) {
			tmr.Reset();
			tmr.Start();

			if (!app->core->WindowComponent->Loop())
				continue;

			if (deltaTime) {
				if (!app->Loop(deltaTime))
					break;
				if (app->curState)
					app->curState->Update(deltaTime);
			}

			// TODO: render
			//while (app->core->Update(deltaTime) == HX_WINDOWMINIMIZED)
			//	hx->core->Loop();

			numFrames++;

			deltaTime = app->FPS < 0.0001 ? 1.0f / 60.0f : 1.0f / app->FPS;

			if (app->maxFPS > 0.001) {
				float maxDeltaTime = 1.0f / app->maxFPS; // delta time at max FPS
				if (deltaTime < maxDeltaTime) {
					WTimer sleepTimer(W_TIMER_SECONDS);
					sleepTimer.Start();
					while (sleepTimer.GetElapsedTime() < (maxDeltaTime - deltaTime));
					deltaTime = maxDeltaTime;
				}
			}

			if (fpsChangeTmr.GetElapsedTime() >= 0.5f) //0.5 second passed -> update FPS
			{
				app->FPS = (float)numFrames * 2.0f;
				numFrames = 0;
				fpsChangeTmr.Reset();
			}
		}

		app->Cleanup();
	}

	core->WindowComponent->Cleanup();

	W_SAFE_DELETE(core);

	WUnInitializeTimers();

	return 0;
}

void Wasabi::SwitchState(WGameState* state) {
	if (curState) {
		curState->Cleanup();
		W_SAFE_DELETE(curState);
	}
	curState = state;
	if (curState)
		curState->Load();
}

