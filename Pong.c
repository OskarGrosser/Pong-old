#include "Pong.h"
#include <windows.h>
#include <ntsecapi.h>

// See <ntsecapi.h> or https://learn.microsoft.com/en-us/windows/win32/api/ntsecapi/nf-ntsecapi-rtlgenrandom
typedef BOOLEAN (__stdcall *LPFNRTLGENRANDOM)(_Out_writes_bytes_(RandomBufferLength) PVOID RandomBuffer, _In_ ULONG RandomBufferLength);

void PaintGame(_In_ HDC hDc, _In_ LPPONGDATA lpPongData) {
	HPEN hPen = CreatePen(PS_DASH, 1, RGB(127, 127, 127));
	HGDIOBJ hBrush = GetStockObject(WHITE_BRUSH);
	RECT paintRect = { 0 };

	SelectObject(hDc, hPen);
	SelectObject(hDc, hBrush);
	SelectObject(hDc, GetStockObject(ANSI_FIXED_FONT));
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
	paintRect.top = lpPongData->paddles[0].offset;
	paintRect.bottom = lpPongData->paddles[0].offset + OG_PADDLE_HEIGHT;
	FillRect(hDc, &paintRect, hBrush);

	paintRect.left = lpPongData->screenWidth - OG_GOAL_INSET;
	paintRect.right = lpPongData->screenWidth - OG_GOAL_INSET + OG_PADDLE_WIDTH;
	paintRect.top = lpPongData->paddles[1].offset;
	paintRect.bottom = lpPongData->paddles[1].offset + OG_PADDLE_HEIGHT;
	FillRect(hDc, &paintRect, hBrush);

	// Ball
	paintRect.left = lpPongData->ball.x;
	paintRect.right = lpPongData->ball.x + OG_BALL_SIZE;
	paintRect.top = lpPongData->ball.y;
	paintRect.bottom = lpPongData->ball.y + OG_BALL_SIZE;
	FillRect(hDc, &paintRect, hBrush);

	// Score
	SetTextColor(hDc, RGB(127, 127, 127));

	WCHAR scoreText[4] = { 0 };
	_itow_s(lpPongData->scores[0], scoreText, _countof(scoreText), 10);
	paintRect.left = 0;
	paintRect.right = OG_GOAL_INSET;
	paintRect.top = 0;
	paintRect.bottom = lpPongData->screenHeight;
	DrawTextW(hDc, scoreText, -1, &paintRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	_itow_s(lpPongData->scores[1], scoreText, _countof(scoreText), 10);
	paintRect.left = lpPongData->screenWidth - OG_GOAL_INSET;
	paintRect.right = lpPongData->screenWidth;
	DrawTextW(hDc, scoreText, -1, &paintRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	DeleteObject(hPen);
}

void UpdateGame(_Inout_ LPPONGDATA lpPongData) {
	const INT minOffset = 0;
	const INT maxOffset = lpPongData->screenHeight - OG_PADDLE_HEIGHT;

	LPPLAYERINPUT inputs = (LPPLAYERINPUT) &lpPongData->inputs;
	LPPADDLE paddles = (LPPADDLE) &lpPongData->paddles;

	for (size_t i = 0; i < 2; ++i) {
		INT direction = 0;
		if (inputs[i].up) --direction;
		if (inputs[i].down) ++direction;

		paddles[i].offset += direction * OG_PADDLE_SPEED;
		if (paddles[i].offset < minOffset) paddles[i].offset = minOffset;
		if (paddles[i].offset > maxOffset) paddles[i].offset = maxOffset;
	}

	LPBALL lpBall = &lpPongData->ball;
	lpBall->x += lpBall->vx;
	lpBall->y += lpBall->vy;

	// Wall bounce
	if (lpBall->y < 0) {
		lpBall->y *= -1;
		lpBall->vy *= -1;
	}
	if (lpBall->y >= lpPongData->screenHeight - OG_BALL_SIZE) {
		INT height = lpPongData->screenHeight - OG_BALL_SIZE;

		lpBall->y = 2 * height - lpBall->y;
		lpBall->vy *= -1;
	}

	// Paddle bounce
	const INT goals[2] = { OG_GOAL_INSET, lpPongData->screenWidth - OG_GOAL_INSET };

	if (lpBall->x <= goals[0]) {
		BOOL isBlocked = lpBall->y <= paddles[0].offset + OG_PADDLE_HEIGHT
			&& lpBall->y + OG_BALL_SIZE >= paddles[0].offset;

		if (isBlocked) {
			if (inputs[0].up) --lpBall->vy;
			if (inputs[0].down) ++lpBall->vy;

			lpBall->x = 2 * goals[0] - lpBall->x;
			lpBall->vx = -lpBall->vx + 1;
		} else {
			++lpPongData->scores[1];
			ResetGame(lpPongData);
		}
	}

	if (lpBall->x + OG_BALL_SIZE > goals[1]) {
		BOOL isBlocked = lpBall->y <= paddles[1].offset + OG_PADDLE_HEIGHT
			&& lpBall->y + OG_BALL_SIZE >= paddles[1].offset;

		if (isBlocked) {
			if (inputs[1].up) --lpBall->vy;
			if (inputs[1].down) ++lpBall->vy;

			lpBall->x = 2 * (goals[1] - OG_BALL_SIZE) - lpBall->x;
			lpBall->vx = -lpBall->vx - 1;
		} else {
			++lpPongData->scores[0];
			ResetGame(lpPongData);
		}
	}
}

int ResetGame(_Inout_ LPPONGDATA lpPongData) {
	static BOOL isInitialized = FALSE;
	static HMODULE hAdvapi32;
	static LPFNRTLGENRANDOM ogRtlGenRandom;

	if (!isInitialized) {
		hAdvapi32 = LoadLibraryW(L"Advapi32.dll");
		if (!hAdvapi32) {
			MessageBoxW(NULL, L"Couldn't load the Advapi32.dll library.", L"Missing Library", MB_OK);
			return 0;
		}

		ogRtlGenRandom = (LPFNRTLGENRANDOM)GetProcAddress(hAdvapi32, "SystemFunction036");
		if (!ogRtlGenRandom) {
			MessageBoxW(NULL, L"Couldn't load RtlGenRandom (SystemFunction036) from the Advapi32.dll library.", L"Missing Function", MB_OK);
			return 0;
		}

		isInitialized = TRUE;
	}

	// Reset paddles
	lpPongData->paddles[0].offset = lpPongData->paddles[1].offset
		= (lpPongData->screenHeight - OG_PADDLE_HEIGHT) / 2;

	// Reset ball
	lpPongData->ball.x = (lpPongData->screenWidth - OG_BALL_SIZE) / 2;
	lpPongData->ball.y = (lpPongData->screenHeight - OG_BALL_SIZE) / 2;

	lpPongData->ball.vx = 2;
	ogRtlGenRandom(&lpPongData->ball.vy, sizeof(lpPongData->ball.vy));
	lpPongData->ball.vy = lpPongData->ball.vy % 5 - 2;

	return 1;
}