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

/* Game */
#define BAR_SPEED 15
#define WINNING_X_COORDINATE 320
#define LOSING_X_COORDINATE 7
#define SCREEN_HEIGHT 240
#define SCREEN_WIDTH 320

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
	unsigned int pos_x;
	unsigned int pos_y;
	unsigned int direction;
} Ball;

typedef struct _game
{
	GameState currentState;
	Ball ball;
	Block *blocks;
	unsigned int bar_pos_y;
} Game;

/* Globals */
Game global = {
		.currentState = Stopped,
};
void game_init(Game *game)
{
	game->currentState = Stopped;
	game->ball.pos_x = width / 2;
	game->ball.pos_y = height / 2;
	game->bar_pos_y = height / 2;
}

/* DISPLAY LOGIC */

/* IO: VGA */

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

/* Display */
#define BAR_WIDTH 7
#define BAR_SEGMENT_HEIGHT 15

void draw_block(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned short color)
{
	for (unsigned int dx = 0; dx < width; dx++)
	{
		for (unsigned int dy = 0; dy < height; dy++)
		{
			SetPixel(x + dx, y + dy, color);
		}
	}
}

void draw_bar(unsigned int y)
{
	draw_block(0, y, BAR_WIDTH, BAR_SEGMENT_HEIGHT, red);
	draw_block(0, y + BAR_SEGMENT_HEIGHT, BAR_WIDTH, BAR_SEGMENT_HEIGHT, green);
	draw_block(0, y + 2 * BAR_SEGMENT_HEIGHT, BAR_WIDTH, BAR_SEGMENT_HEIGHT, red);
}

void draw_ball()
{
	if (global.ball.pos_x >= 0 && global.ball.pos_x < width - 7 && global.ball.pos_y >= 0 && global.ball.pos_y < height - 7)
	{
		draw_block(global.ball.pos_x + 3, global.ball.pos_y, 1, 1, black);
		draw_block(global.ball.pos_x + 2, global.ball.pos_y + 1, 3, 1, black);
		draw_block(global.ball.pos_x + 1, global.ball.pos_y + 2, 5, 1, black);
		draw_block(global.ball.pos_x, global.ball.pos_y + 3, 7, 1, black);
		draw_block(global.ball.pos_x + 1, global.ball.pos_y + 4, 5, 1, black);
		draw_block(global.ball.pos_x + 2, global.ball.pos_y + 5, 3, 1, black);
		draw_block(global.ball.pos_x + 3, global.ball.pos_y + 6, 1, 1, black);
	}
}

/* GAME LOGIC */

/* IO: UART */

int ReadUart();
asm("ReadUart:\n\t"
		"LDR R1, =0xFF201000 \n\t"
		"LDR R0, [R1]\n\t"
		"BX LR");

void WriteUart(char c);
asm("WriteUart: \n\t"
		"LDR R1, =0xFF201000 \n\t"
		"STRH R0, [R1]\n\t"
		"BX LR");

char UartGetChar()
{
	unsigned long long out = ReadUart();
	char c = '\0';

	if (out & 0x8000 && c != '\0')
	{
		// store first character
		c = ((char)out & 0xFF);
	}

	// drain buffer
	int remaining = 0;
	while (remaining > 0)
	{
		/* code */
		remaining = (out & 0xFF0000) >> 4;
	}

	return c;
}

void write(char *str)
{
	for (int i = 0; str[i] != '\0'; i++)
		WriteUart(str[i]);
}

// It must initialize the game
void reset()
{
	// TODO: You might want to reset other state in here
	game_init(&global);
	ClearScreen();
}

int main(int argc, char *argv[])
{
	if (NCOLS < 1 || 18 < NCOLS)
	{
		// TODO
		/* Inform user this is not a configurable state */
		write("Invalid number of columns! NCOLS must be at least 1 and at most 18");
		return 1;
	}

	reset();

	// HINT: This loop allows the user to restart the game after loosing/winning the previous game
	while (1)
	{
		switch (global.currentState)
		{
		case Stopped:
		{
			/* Start screen */
			write("Press 'w' or 's' to start game\n");
			while (global.currentState == Stopped)
			{
				unsigned long long out = ReadUart();
				switch ((out & 0xFF))
				{
				case 0x77:
				case 0x73:
					global.currentState = Running;
					break;

				default:
					break;
				}
			}

			break;
		}
		case Running:
		{
			/* Game running */
			/* Check for win/lose condition */
			if (global.ball.pos_x < LOSING_X_COORDINATE)
			{
				global.currentState = Lost;
				continue;
			}
			if (WINNING_X_COORDINATE < global.ball.pos_x)
			{
				global.currentState = Won;
				continue;
			}

			/* IO Update bar */
			{
				unsigned long long out = ReadUart();
				if ((out & 0x8000))
				{
					if ((out & 0xFF) == 0x77)
					{
						global.bar_pos_y -= BAR_SPEED;
					}
					if ((out & 0xFF) == 0x73)
					{
						global.bar_pos_y += BAR_SPEED;
					}
				}
			}

			/* Update game state */
			draw_ball();
			draw_bar(global.bar_pos_y);

			// TODO: Update balls position and direction
			// physics_update();

			// TODO: Hit Check with Blocks
			// HINT: try to only do this check when we potentially have a hit, as it is relatively expensive and can slow down game play a lot

			/* Update screen */
			break;
		}
		case Won:
		{
			for (unsigned int i = 0; i < 1000000; i++)
			{
				write(won);
			}
			global.currentState = Stopped;
			reset();

			break;
		}
		case Lost:
		{
			for (unsigned int i = 0; i < 1000000; i++)
			{
				write(lost);
			}
			global.currentState = Stopped;
			reset();

			break;
		}
		case Exit:
		default:
			return 0;
		}
	}
	return 0;
}
