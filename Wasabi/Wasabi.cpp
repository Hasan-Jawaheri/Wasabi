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
	app->core = core;

	if (app->Setup()) {
		while (!core->__EXIT) {
			if (core->WindowComponent->Loop()) {
				if (!app->Loop(1.0f / 60.0f /* PLACEHOLDER */))
					break;
				// TODO: render
			}
		}
	}
	app->Cleanup();
	core->WindowComponent->Cleanup();

	W_SAFE_DELETE(core);

	return 0;
}
