#ifdef _WIN32

#include "WWC_Win32.h"
#include "../../Input/Windows/WIC_Win32.h"

LRESULT CALLBACK hMainWndProc(HWND hWnd, uint msg, WPARAM wParam, LPARAM lParam);

WWC_Win32::WWC_Win32(Wasabi* app) : WWindowComponent(app) {
	m_mainWindow = nullptr;
	m_hInstance = GetModuleHandleA(nullptr);
	m_minWindowX = 640 / 4;
	m_minWindowY = 480 / 4;
	m_maxWindowX = 640 * 20;
	m_maxWindowY = 480 * 20;
	m_isMinimized = true;

	app->engineParams.insert(std::pair<std::string, void*>("classStyle", (void*)(CS_HREDRAW | CS_VREDRAW)));
	app->engineParams.insert(std::pair<std::string, void*>("classStyle", (void*)(CS_HREDRAW | CS_VREDRAW))); // uint
	app->engineParams.insert(std::pair<std::string, void*>("classIcon", (void*)(NULL))); // HICON
	app->engineParams.insert(std::pair<std::string, void*>("classCursor", (void*)(LoadCursorA(NULL, MAKEINTRESOURCEA(32512))))); // HCURSOR
	app->engineParams.insert(std::pair<std::string, void*>("menuName", (void*)(NULL))); // LPCSTR
	app->engineParams.insert(std::pair<std::string, void*>("menuProc", (void*)(NULL))); // void (*) (HMENU, uint)
	app->engineParams.insert(std::pair<std::string, void*>("classIcon_sm", (void*)(NULL))); // HICON
	app->engineParams.insert(std::pair<std::string, void*>("windowMenu", (void*)(NULL))); // HMENU
	app->engineParams.insert(std::pair<std::string, void*>("windowParent", (void*)(NULL))); // HWND
	app->engineParams.insert(std::pair<std::string, void*>("windowStyle", (void*)(WS_CAPTION | WS_OVERLAPPEDWINDOW | WS_VISIBLE))); // uint
	app->engineParams.insert(std::pair<std::string, void*>("windowStyleEx", (void*)(WS_EX_OVERLAPPEDWINDOW))); // uint
	app->engineParams.insert(std::pair<std::string, void*>("defWndX", (void*)(-1))); // int
	app->engineParams.insert(std::pair<std::string, void*>("defWndY", (void*)(-1))); //int
}
WError WWC_Win32::Initialize(int width, int height) {
	//do not initialize if the window is already there
	if (m_mainWindow) {
		//build window rect from client rect (client rect = directx rendering rect)
		RECT rc;
		rc.left = 0;
		rc.top = 0;
		rc.right = width;
		rc.bottom = height;
		AdjustWindowRectEx(&rc, (uint)m_app->engineParams["windowStyle"],
			(m_app->engineParams["windowMenu"] == nullptr) ? FALSE : TRUE,
			(uint)m_app->engineParams["windowStyleEx"]);

		//adjust window position (no window re-creation)
		SetWindowPos(m_mainWindow, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE);

		return WError(W_SUCCEEDED);
	}

	//create window class
	WNDCLASSEXA wcex;

	wcex.cbSize = sizeof(WNDCLASSEXA);
	wcex.style = (uint)m_app->engineParams["classStyle"];
	wcex.lpfnWndProc = hMainWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_hInstance;
	wcex.hIcon = (HICON)m_app->engineParams["classIcon"];
	wcex.hCursor = (HCURSOR)m_app->engineParams["classCursor"];
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = (LPCSTR)m_app->engineParams["menuName"];
	wcex.lpszClassName = "WWndClasse";
	wcex.hIconSm = (HICON)m_app->engineParams["classIcon_sm"];

	//keep trying to register the class until a valid name exist (for multiple cores)
	if (!RegisterClassExA(&wcex))
		return WError(W_CLASSREGISTERFAILED);

	//adjust the size, the provided sizes (width,height) should be the client area sizes, so build the window size from the client size
	RECT rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = width;
	rc.bottom = height;
	AdjustWindowRectEx(&rc, (uint)m_app->engineParams["windowStyle"],
		(m_app->engineParams["windowMenu"] == nullptr) ? FALSE : TRUE,
		(uint)m_app->engineParams["windowStyleEx"]);

	//set to default positions if specified
	int x, y;
	if ((int)m_app->engineParams["defWndX"] == -1 && (int)m_app->engineParams["defWndY"] == -1) {
		x = (GetSystemMetrics(SM_CXSCREEN) / 2) - ((rc.right - rc.left) / 2);
		y = (GetSystemMetrics(SM_CYSCREEN) / 2) - ((rc.bottom - rc.top) / 2);
	}
	else {
		x = (int)m_app->engineParams["defWndX"];
		y = (int)m_app->engineParams["defWndY"];
	}

	//
	//Create the main window
	//
	m_mainWindow = CreateWindowExA((uint)m_app->engineParams["windowStyleEx"], wcex.lpszClassName,
		(LPCSTR)m_app->engineParams["appName"],
		(uint)m_app->engineParams["windowStyle"],
		x, y, rc.right - rc.left, rc.bottom - rc.top,
		(HWND)m_app->engineParams["windowParent"], (HMENU)m_app->engineParams["windowMenu"], m_hInstance, nullptr);

	GetWindowRect(m_mainWindow, &rc);
	if (!m_mainWindow) //error creating the window
		return WError(W_WINDOWNOTCREATED);

	//give the main window's procedure an instance of the core
	SetWindowLongA(m_mainWindow, GWL_USERDATA, (LONG)(void*)m_app);

	m_isMinimized = false;

	return WError(W_SUCCEEDED);
}
bool WWC_Win32::Loop() {
	MSG msg;
	while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) //find messages
	{
		//process the message
		TranslateMessage(&msg);
		DispatchMessageA(&msg);

		//if a quit message is received, quit
		if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY) {
			//exit
			m_app->__EXIT = true;
			return false;
		}
	}
	return !m_isMinimized;
}
void WWC_Win32::Cleanup() {
	if (m_mainWindow)
		DestroyWindow(m_mainWindow);
}

void WWC_Win32::SetWindowTitle(const char* const title) {
	//set the title of the main window
	SetWindowTextA(m_mainWindow, title);
}
void WWC_Win32::SetWindowPosition(int x, int y) {
	//set the position of the main window
	SetWindowPos(m_mainWindow, nullptr, x, y, 0, 0, SWP_NOSIZE);
}
void WWC_Win32::SetWindowSize(int width, int height) {
	//manual window resizing

	//save old sizes
	RECT rc;
	GetClientRect(m_mainWindow, &rc);
	int oldSizeX = rc.right;
	int oldSizeY = rc.bottom;

	//change main window size
	SetWindowPos(m_mainWindow, nullptr, 0, 0, width, height, SWP_NOMOVE);
	GetClientRect(m_mainWindow, &rc);

	//re-initialize the core to fit the new size
	// TODO: resize engine
	//m_app->Init(rc.right, rc.bottom, m_app->IsVSyncEnabled());
}
void WWC_Win32::MaximizeWindow() {
	//show window as maximized
	ShowWindow(m_mainWindow, SW_SHOWMAXIMIZED);
	m_isMinimized = false; //mark not minimized
}
void WWC_Win32::MinimizeWindow() {
	//show window as minimized
	ShowWindow(m_mainWindow, SW_SHOWMINIMIZED);
	m_isMinimized = true; //mark minimized
}
uint WWC_Win32::RestoreWindow() {
	//restore window
	int result = ShowWindow(m_mainWindow, SW_RESTORE);
	m_isMinimized = false; //mark not minimized
	return result;
}
uint WWC_Win32::GetWindowWidth() const {
	RECT rc;
	GetClientRect(m_mainWindow, &rc);
	return rc.right;
}
uint WWC_Win32::GetWindowHeight() const {
	RECT rc;
	GetClientRect(m_mainWindow, &rc);
	return rc.bottom;
}
int WWC_Win32::GetWindowPositionX() const {
	//return the left coordinate of the window rect
	RECT rc;
	GetWindowRect(m_mainWindow, &rc);
	return rc.left;
}
int WWC_Win32::GetWindowPositionY() const {
	//return the top coordinate of the window rect
	RECT rc;
	GetWindowRect(m_mainWindow, &rc);
	return rc.top;
}
HWND WWC_Win32::GetWindow() const {
	//return the API HWND
	return m_mainWindow;
}
HINSTANCE WWC_Win32::GetInstance() const {
	//return the instance
	return m_hInstance;
}
void* WWC_Win32::GetPlatformHandle() const {
	return (void*)m_hInstance;
}
void* WWC_Win32::GetWindowHandle() const {
	return (void*)m_mainWindow;
}
void WWC_Win32::SetFullScreenState(bool bFullScreen) {
	//set the fullscreen state of the window
	// TODO: implement this
	//m_app->GetSwapChain()->SetFullscreenState(bFullScreen, nullptr);
}
bool WWC_Win32::GetFullScreenState() const {
	BOOL bf = false;
	// TODO: implement this
	//m_app->GetSwapChain()->GetFullscreenState(&bf, nullptr);
	return (bool)bf;
}
void WWC_Win32::SetWindowMinimumSize(int minX, int minY) {
	m_minWindowX = minX;
	m_minWindowY = minY;
}
void WWC_Win32::SetWindowMaximumSize(int maxX, int maxY) {
	m_maxWindowX = maxX;
	m_maxWindowY = maxY;
}

//window procedure
LRESULT CALLBACK hMainWndProc(HWND hWnd, uint msg, WPARAM wParam, LPARAM lParam) {
	//get an inctance of the core
	Wasabi* appInst = (Wasabi*)GetWindowLong(hWnd, GWL_USERDATA);

	//return default behavior if the instance is not set yet
	if (!appInst) {
		if (msg == WM_GETMINMAXINFO) {
			//set the min/max window size
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 10;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 10;
			((MINMAXINFO*)lParam)->ptMaxTrackSize.x = 99999;
			((MINMAXINFO*)lParam)->ptMaxTrackSize.y = 99999;
			return TRUE;
		}
		return DefWindowProcA(hWnd, msg, wParam, lParam);
	}

	LRESULT result = TRUE;

	switch (msg) {
		//quit
	case WM_CLOSE:
	case WM_QUIT:
	case WM_DESTROY:
		appInst->__EXIT = true;
		break;

	case WM_KEYDOWN:
		if (!appInst->InputComponent)
			break;
		//ESCAPE KEY will close the application if m_escapeE is true
		if (wParam == VK_ESCAPE && ((WIC_Win32*)appInst->InputComponent)->m_escapeE)
			appInst->__EXIT = true;
		appInst->InputComponent->InsertRawInput(wParam, true);
		if (appInst->curState)
			appInst->curState->OnKeydown(wParam);
		break;
	case WM_KEYUP:
		if (!appInst->InputComponent)
			break;
		appInst->InputComponent->InsertRawInput(wParam, false);
		if (appInst->curState)
			appInst->curState->OnKeyup(wParam);
		break;
	case WM_CHAR:
		if (appInst->curState)
			appInst->curState->OnInput(wParam);
		break;
	case WM_GETMINMAXINFO:
		//set the min/max window size
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = ((WWC_Win32*)appInst->WindowComponent)->m_minWindowX;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = ((WWC_Win32*)appInst->WindowComponent)->m_minWindowY;
		((MINMAXINFO*)lParam)->ptMaxTrackSize.x = ((WWC_Win32*)appInst->WindowComponent)->m_maxWindowX;
		((MINMAXINFO*)lParam)->ptMaxTrackSize.y = ((WWC_Win32*)appInst->WindowComponent)->m_maxWindowY;
		break;
	case WM_SIZE:
	{
		//flag the core as minimized to prevent rendering
		if (wParam == SIZE_MINIMIZED)
			((WWC_Win32*)appInst->WindowComponent)->m_isMinimized = true;
		else {
			((WWC_Win32*)appInst->WindowComponent)->m_isMinimized = false; //flag core as not minimized to allow rendering
			RECT rc;
			GetClientRect(hWnd, &rc); //get window client dimensions

										//re-initialize the core to fit the new size
			if (!appInst->Resize(rc.right - rc.left, rc.bottom - rc.top))
				return TRUE;
		}
		break;
	}
	//mouse input update
	case WM_LBUTTONDOWN:
		if (appInst->InputComponent)
			((WIC_Win32*)appInst->InputComponent)->m_leftClick = true;
		break;
	case WM_LBUTTONUP:
		if (appInst->InputComponent)
			((WIC_Win32*)appInst->InputComponent)->m_leftClick = false;
		break;
	case WM_RBUTTONDOWN:
		if (appInst->InputComponent)
			((WIC_Win32*)appInst->InputComponent)->m_rightClick = true;
		break;
	case WM_RBUTTONUP:
		if (appInst->InputComponent)
			((WIC_Win32*)appInst->InputComponent)->m_rightClick = false;
		break;
	case WM_MBUTTONDOWN:
		if (appInst->InputComponent)
			((WIC_Win32*)appInst->InputComponent)->m_middleClick = true;
		break;
	case WM_MBUTTONUP:
		if (appInst->InputComponent)
			((WIC_Win32*)appInst->InputComponent)->m_middleClick = false;
		break;
	case WM_MOUSEMOVE:
		if (appInst->InputComponent) {
			//((WIC_Win32*)appInst->InputComponent)->m_mouseX = LOWORD(lParam);
			//((WIC_Win32*)appInst->InputComponent)->m_mouseY = HIWORD(lParam);
		}
		break;
	case WM_MOUSEWHEEL:
	{
		if (appInst->InputComponent) {
			int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			((WIC_Win32*)appInst->InputComponent)->m_mouseZ += delta;
		}
		break;
	}
	case WM_COMMAND:
	{
		//send messages to child windows
		if (HIWORD(wParam) == 0)
			if (appInst->engineParams["windowMenu"] != nullptr && appInst->engineParams["menuPorc"] != nullptr)
				((void(*)(HMENU, uint))appInst->engineParams["menuProc"])(GetMenu(hWnd), LOWORD(wParam));
		break;
	}
	default: //default behavior for other messages
		result = DefWindowProcA(hWnd, msg, wParam, lParam);
	}

	return result;
}

#endif
