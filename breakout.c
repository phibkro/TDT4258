/***************************************************************************************************
 * DON'T REMOVE THE VARIABLES BELOW THIS COMMENT                                                   *
 **************************************************************************************************/
unsigned long long __attribute__((used)) VGAaddress = 0xc8000000; // Memory storing pixels
unsigned int __attribute__((used)) red = 0x0000F0F0;
unsigned int __attribute__((used)) green = 0x00000F0F;
unsigned int __attribute__((used)) blue = 0x000000FF;
unsigned int __attribute__((used)) white = 0x0000FFFF;
unsigned int __attribute__((used)) black = 0x0;

// Don't change the name of this variables
#define NCOLS 10		 // <- Supported value range: [1,18]
#define NROWS 14		 // <- This variable might change.
#define TILE_SIZE 15 // <- Tile size, might change.

char *won = "You Won";																				 // DON'T TOUCH THIS - keep the string as is
char *lost = "You Lost";																			 // DON'T TOUCH THIS - keep the string as is
unsigned short height = 240;																	 // DON'T TOUCH THIS - keep the value as is
unsigned short width = 320;																		 // DON'T TOUCH THIS - keep the value as is
char font8x8[128][8];																					 // DON'T TOUCH THIS - this is a forward declaration
unsigned char tiles[NROWS][NCOLS] __attribute__((used)) = {0}; // DON'T TOUCH THIS - this is the tile map
/**************************************************************************************************/

/* Utility */
#define true 1
#define false 0

/* IO */

/* VGA */

void ClearScreen();
asm("ClearScreen: \n\t"
		"PUSH {R4-R6,LR} \n \t"
		"LDR R3, =VGAaddress \n\t"
		"LDR R3, [R3] \n\t"
		"MOV r0, #0 \n\t"
		"MOV r1, #0 \n\t"					 // x counter
		"MOV r2, #0 \n\t"					 // y counter
		"MOV r4, #0 \n\t"					 // x offset
		"MOV r5, #0 \n\t"					 // y offset
		"LDR r6, =0x0000FFFF \n\t" // color
		"clearscreen_loop: \n\t"
		"LSL r4, r1, #1 \n\t"
		"LSL r5, r2, #10 \n\t"
		"MOV r0, #0 \n\t"
		"ADD r0, r4, r5 \n\t"
		"STRH r6, [R3, R0] \n\t"
		"ADD r1, #1 \n\t"
		"CMP r1, #320 \n\t"
		"BLT clearscreen_loop \n\t" /* increment y */
		"MOV r1, #0 \n\t"
		"ADD r2, #1 \n\t"
		"CMP r2, #240 \n\t"
		"BLT clearscreen_loop \n\t"
		"POP {R4-R6,LR} \n\t"
		"BX LR");

void SetPixel(unsigned int x_coord, unsigned int y_coord, unsigned int color);
// assumes R0 = x-coord, R1 = y-coord, R2 = colorvalue
asm("SetPixel: \n\t"
		"LDR R3, =VGAaddress \n\t"
		"LDR R3, [R3] \n\t"
		"LSL R1, R1, #10 \n\t"
		"LSL R0, R0, #1 \n\t"
		"ADD R1, R0 \n\t"
		"STRH R2, [R3,R1] \n\t"
		"BX LR");

/* UART */

int UartRead();
asm("UartRead:\n\t"
		"LDR R1, =0xFF201000 \n\t"
		"LDR R0, [R1]\n\t"
		"BX LR");

char UartGetChar()
{
	unsigned long long out = UartRead();

	if (out & 0x8000)
	{
		// valid
		return ((char)out & 0xFF);
	}
	else
	{
		return '\0';
	}
}

void uart_drain_buffer()
{
	int remaining = 0;
	do
	{
		unsigned long long out = UartRead();
		if (!(out & 0x8000))
		{
			// not valid - abort reading
			return;
		}
		remaining = (out & 0xFF0000) >> 4;
	} while (remaining > 0);
}

void UartWrite(char c);
asm("UartWrite: \n\t"
		"LDR R1, =0xFF201000 \n\t"
		"STRH R0, [R1]\n\t"
		"BX LR");

void write(char *str)
{
	for (int i = 0; str[i] != '\0'; i++)
		UartWrite(str[i]);
}

/* Game */
#define BAR_SPEED 15
#define WINNING_X_COORDINATE 320
#define LOSING_X_COORDINATE 7
#define SCREEN_HEIGHT 240
#define SCREEN_WIDTH 320

unsigned int bitmap[SCREEN_HEIGHT * SCREEN_WIDTH];

typedef enum _gameState
{
	Stopped = 0,
	Running = 1,
	Won = 2,
	Lost = 3,
	Exit = 4,
} GameState;

#define BRICK_SIZE 15
typedef struct _block
{
	unsigned char destroyed;
	unsigned char deleted;
	unsigned int pos_x;
	unsigned int pos_y;
	unsigned int color;
} Block;

#define BALL_DIAMETER 7
typedef struct _ball
{
	unsigned int x_coord;
	unsigned int y_coord;
} Ball;

typedef struct _game
{
	GameState currentState;
	Ball ball;
	Block *blocks;
	int bar_y_coord;
} Game;

void game_init(Game *game) {
	game->currentState = Stopped;
	game->ball.x_coord = SCREEN_WIDTH / 2;
	game->ball.y_coord = SCREEN_WIDTH / 2;
	game->bar_y_coord = SCREEN_HEIGHT / 2;
}

/* Globals */
Game global = {
		.currentState = Stopped,
		.ball = {
				.x_coord = SCREEN_WIDTH / 2,
				.y_coord = SCREEN_HEIGHT / 2,
		},
		.bar_y_coord = SCREEN_HEIGHT / 2,
};

// It must initialize the game
void reset()
{
	uart_drain_buffer();

	// TODO: You might want to reset other state in here
	game_init(&global);
}

int main(int argc, char *argv[])
{
	write("hello world\n");
	return 0;
}
