#include <windows.h>
#include <crtdbg.h>

#define OG_TARGET_FREQUENCY 60

#define OG_PADDLE_HEIGHT 80
#define OG_PADDLE_WIDTH 10
#define OG_PADDLE_SPEED 4

#define OG_GOAL_INSET 50

typedef struct ogPaddle {
	INT offset;
} PADDLE, *LPPADDLE;

typedef struct ogPlayerInput {
	BOOL up;
	BOOL down;
} PLAYERINPUT, *LPPLAYERINPUT;

typedef struct ogPongData {
	LONG screenWidth;
	LONG screenHeight;
	UINT scores[2];
	PADDLE paddles[2];
	PLAYERINPUT inputs[2];
} PONGDATA, *LPPONGDATA;

void PaintGame(_In_ HDC hDc, _In_ LPPONGDATA lpPongData) {
	HPEN hPen = CreatePen(PS_DASH, 1, RGB(127, 127, 127));
	HGDIOBJ hBrush = GetStockObject(WHITE_BRUSH);
	RECT paintRect = { 0 };

	SelectObject(hDc, hPen);
	SelectObject(hDc, GetStockObject(NULL_BRUSH));
	SetBkMode(hDc, TRANSPARENT);

	// Background
	paintRect.left = 0;
	paintRect.right = lpPongData->screenWidth;
	paintRect.top = 0;
	paintRect.bottom = lpPongData->screenHeight;
	FillRect(hDc, &paintRect, GetStockObject(BLACK_BRUSH));

	// Goal lines
	MoveToEx(hDc, OG_GOAL_INSET, 0, NULL);
	LineTo(hDc, OG_GOAL_INSET, lpPongData->screenHeight);

	MoveToEx(hDc, lpPongData->screenWidth - OG_GOAL_INSET, 0, NULL);
	LineTo(hDc, lpPongData->screenWidth - OG_GOAL_INSET, lpPongData->screenHeight);

	// Paddles
	paintRect.left = OG_GOAL_INSET - OG_PADDLE_WIDTH;
	paintRect.right = OG_GOAL_INSET;
	paintRect.top = lpPongData->paddles[0].offset + (lpPongData->screenHeight - OG_PADDLE_HEIGHT) / 2;
	paintRect.bottom = lpPongData->paddles[0].offset + (lpPongData->screenHeight + OG_PADDLE_HEIGHT) / 2;
	FillRect(hDc, &paintRect, hBrush);

	paintRect.left = lpPongData->screenWidth - OG_GOAL_INSET;
	paintRect.right = lpPongData->screenWidth - OG_GOAL_INSET + OG_PADDLE_WIDTH;
	paintRect.top = lpPongData->paddles[1].offset + (lpPongData->screenHeight - OG_PADDLE_HEIGHT) / 2;
	paintRect.bottom = lpPongData->paddles[1].offset + (lpPongData->screenHeight + OG_PADDLE_HEIGHT) / 2;
	FillRect(hDc, &paintRect, hBrush);

	DeleteObject(hPen);
}

LRESULT CALLBACK PongWindowProc(
	_In_ HWND hWnd,
	_In_ UINT uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
) {
	switch (uMsg) {
	case WM_NCCREATE:
		{
			// Resize client area to window size
			RECT windowRect;
			if (!GetWindowRect(hWnd, &windowRect)) {
				return FALSE;
			}

			WINDOWINFO windowInfo;
			windowInfo.cbSize = sizeof(windowInfo);
			if (!GetWindowInfo(hWnd, &windowInfo)) {
				return FALSE;
			}

			HMENU hMenu = GetMenu(hWnd);

			if (!AdjustWindowRectEx(&windowRect, windowInfo.dwStyle, hMenu != NULL, windowInfo.dwExStyle)) {
				return FALSE;
			}

			int clientWidth = windowRect.right - windowRect.left;
			int clientHeight = windowRect.bottom - windowRect.top;
			UINT uFlags = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE;
			if (!SetWindowPos(hWnd, NULL, 0, 0, clientWidth, clientHeight, uFlags)) {
				return FALSE;
			}
		}
		break;
	case WM_CREATE:
		{
			LPPONGDATA lpData = ((LPCREATESTRUCTW)lParam)->lpCreateParams;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, lpData);
		}
		break;
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
		{
			LPPONGDATA lpData = GetWindowLongPtrW(hWnd, GWLP_USERDATA);

			PAINTSTRUCT ps;
			HDC hDc = BeginPaint(hWnd, &ps);

			HDC hBkDc = CreateCompatibleDC(hDc);
			HBITMAP hBitmap = CreateCompatibleBitmap(hDc, lpData->screenWidth, lpData->screenHeight);

			SelectObject(hBkDc, hBitmap);

			PaintGame(hBkDc, lpData);
			BitBlt(hDc, 0, 0, lpData->screenWidth, lpData->screenHeight, hBkDc, 0, 0, SRCCOPY);

			DeleteObject(hBitmap);
			DeleteDC(hBkDc);

			EndPaint(hWnd, &ps);
		}
		return 0;
	case WM_KEYDOWN:
		{
			LPPONGDATA lpData = GetWindowLongPtrW(hWnd, GWLP_USERDATA);

			if (wParam == 'W') {
				lpData->inputs[0].up = TRUE;
			}
			if (wParam == 'S') {
				lpData->inputs[0].down = TRUE;
			}

			if (wParam == VK_UP) {
				lpData->inputs[1].up = TRUE;
			}
			if (wParam == VK_DOWN) {
				lpData->inputs[1].down = TRUE;
			}
		}
		break;
	case WM_KEYUP:
		{
			LPPONGDATA lpData = GetWindowLongPtrW(hWnd, GWLP_USERDATA);

			if (wParam == 'W') {
				lpData->inputs[0].up = FALSE;
			}
			if (wParam == 'S') {
				lpData->inputs[0].down = FALSE;
			}

			if (wParam == VK_UP) {
				lpData->inputs[1].up = FALSE;
			}
			if (wParam == VK_DOWN) {
				lpData->inputs[1].down = FALSE;
			}
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void UpdateGame(_In_ LPPONGDATA lpPongData) {
	INT minOffset = (lpPongData->screenHeight - OG_PADDLE_HEIGHT) / 2;

	for (size_t i = 0; i < 2; ++i) {
		LPPLAYERINPUT input = &lpPongData->inputs[i];
		LPPADDLE paddle = &lpPongData->paddles[i];

		INT direction = 0;
		if (input->up) --direction;
		if (input->down) ++direction;

		paddle->offset += direction * OG_PADDLE_SPEED;
	}

	if (lpPongData->paddles[0].offset) return;
}

int GameLoop(_In_ HWND hPongWindow, _In_ LPPONGDATA lpPongData) {
	LARGE_INTEGER qpFrequency;
	if (!QueryPerformanceFrequency(&qpFrequency)) {
		return 1;
	}

	LONGLONG llCycle = qpFrequency.QuadPart;
	llCycle /= OG_TARGET_FREQUENCY;

	LARGE_INTEGER qpLastCount;
	if (!QueryPerformanceCounter(&qpLastCount)) {
		return 1;
	}

	LONGLONG llCount = 0;

	MSG msg;
	do {
		{
			LARGE_INTEGER qpThisCount;
			if (!QueryPerformanceCounter(&qpThisCount)) {
				return 1;
			}
			llCount += qpThisCount.QuadPart - qpLastCount.QuadPart;
			qpLastCount = qpThisCount;
		}

		LONGLONG llTicks = llCount / llCycle;
		llCount %= llCycle;

		// Input
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				return msg.wParam;
			}

			DispatchMessageW(&msg);
		}

		// Update
		for (LONGLONG i = 0; i < llTicks; ++i) {
			UpdateGame(lpPongData);
		}

		// Render
		InvalidateRgn(hPongWindow, NULL, FALSE);
		if (!UpdateWindow(hPongWindow)) {
			return 1;
		}

		// Sleep
		{
			LONGLONG llRemainder = llCycle - llCount;
			_ASSERT(llRemainder >= 0);

			LONGLONG llRemainderMs = (llRemainder * 1000) / qpFrequency.QuadPart;
			Sleep((DWORD)llRemainderMs);
		}

	} while (msg.message != WM_QUIT); // Should never happen; see return in Input

	return msg.wParam;
}

HWND CreatePongWindow(_In_ HINSTANCE hInstance, _In_ LPPONGDATA lpPongData) {
	_ASSERT(lpPongData);

	WNDCLASSEXW pongClass = { 0 };
	pongClass.cbSize = sizeof(pongClass);
	pongClass.hInstance = hInstance;
	pongClass.lpszClassName = L"ogPongClass";
	pongClass.lpfnWndProc = PongWindowProc;
	pongClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
	if (!RegisterClassExW(&pongClass)) {
		return NULL;
	}

	HWND hPongWindow = CreateWindowExW(
		WS_EX_APPWINDOW,
		pongClass.lpszClassName,
		L"Pong",
		WS_BORDER | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME,
		CW_USEDEFAULT, CW_USEDEFAULT,
		lpPongData->screenWidth, lpPongData->screenHeight,
		NULL, NULL,
		hInstance,
		lpPongData
	);
	if (!hPongWindow) {
		return NULL;
	}

	return hPongWindow;
}

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPCWSTR lpCmdLine,
	_In_ int nCmdShow
) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	PONGDATA pongData = { 0 };
	pongData.screenWidth = 800;
	pongData.screenHeight = 600;
	HWND hPongWindow = CreatePongWindow(hInstance, &pongData);
	if (!hPongWindow) {
		return 1;
	}

	ShowWindow(hPongWindow, nCmdShow);

	int result = GameLoop(hPongWindow, &pongData);

	return result;
}