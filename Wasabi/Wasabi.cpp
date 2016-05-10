#include <Windows.h>
#include "WTimer.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow) {
	WInitializeTimers();
	return 0;
}
