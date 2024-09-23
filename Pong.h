#include <windows.h>

#define OG_TARGET_FREQUENCY 60

#define OG_BALL_SIZE 12

#define OG_PADDLE_HEIGHT 80
#define OG_PADDLE_WIDTH 10
#define OG_PADDLE_SPEED 4

#define OG_GOAL_INSET 50

typedef struct ogBall {
	INT x;
	INT y;
	INT vx;
	INT vy;
} BALL, *LPBALL;

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
	BALL ball;
} PONGDATA, *LPPONGDATA;

void PaintGame(_In_ HDC hDc, _In_ LPPONGDATA lpPongData);
void UpdateGame(_In_ LPPONGDATA lpPongData);