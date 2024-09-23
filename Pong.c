#include "Pong.h"
#include <windows.h>

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

	DeleteObject(hPen);
}

void UpdateGame(_In_ LPPONGDATA lpPongData) {
	const INT minOffset = 0;
	const INT maxOffset = lpPongData->screenHeight - OG_PADDLE_HEIGHT;

	for (size_t i = 0; i < 2; ++i) {
		LPPLAYERINPUT input = &lpPongData->inputs[i];
		LPPADDLE paddle = &lpPongData->paddles[i];

		INT direction = 0;
		if (input->up) --direction;
		if (input->down) ++direction;

		paddle->offset += direction * OG_PADDLE_SPEED;
		if (paddle->offset < minOffset) paddle->offset = minOffset;
		if (paddle->offset > maxOffset) paddle->offset = maxOffset;
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
	LPPADDLE paddles = &lpPongData->paddles;
	const INT goals[2] = { OG_GOAL_INSET, lpPongData->screenWidth - OG_GOAL_INSET };

	if (lpBall->x <= goals[0]) {
		BOOL isBlocked = lpBall->y <= paddles[0].offset + OG_PADDLE_HEIGHT
			&& lpBall->y + OG_BALL_SIZE >= paddles[0].offset;

		if (isBlocked) {
			lpBall->x = 2 * goals[0] - lpBall->x;
			lpBall->vx = -lpBall->vx + 1;
		}
	}

	if (lpBall->x + OG_BALL_SIZE > goals[1]) {
		BOOL isBlocked = lpBall->y <= paddles[1].offset + OG_PADDLE_HEIGHT
			&& lpBall->y + OG_BALL_SIZE >= paddles[1].offset;

		if (isBlocked) {
			lpBall->x = 2 * (goals[1] - OG_BALL_SIZE) - lpBall->x;
			lpBall->vx = -lpBall->vx - 1;
		}
	}
}