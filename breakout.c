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
#define WINNING_X_COORDINATE 320
#define LOSING_X_COORDINATE 7

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

// Direction enums
typedef enum
{
	LEFT = -1,
	RIGHT = 1
} Horizontal;

typedef enum
{
	UP = -1,
	NEUTRAL = 0,
	DOWN = 1
} Vertical;

#define BALL_DIAMETER 7
// Ball state
typedef struct
{
	unsigned int x;
	unsigned int y;
	Horizontal horizontal_direction;
	Vertical vertical_direction;
	unsigned int speed;
	unsigned int diameter;
	unsigned int prev_x;
	unsigned int prev_y;
} Ball;

// Bar state
typedef struct
{
	unsigned int y;
	unsigned int x;
	unsigned int width;
	unsigned int height;
	unsigned int step;
	unsigned int prev_y;
} Bar;

/* Globals */
GameState game_state = Stopped;
Ball ball;
Bar bar;

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

void erase_bar_diff(unsigned int y)
{
	// If moving up, clear the bottom part that was previously occupied
	if (y < bar.prev_y)
	{
		draw_block(0, y + bar.height, bar.width, bar.prev_y - y, white);
	}
	// If moving down, clear the top part that was previously occupied
	else if (y > bar.prev_y)
	{
		draw_block(0, bar.prev_y, bar.width, y - bar.prev_y, white);
	}
}

void draw_bar(unsigned int y)
{
	erase_bar_diff(y);
	unsigned int segment_height = bar.height / 3;
	draw_block(0, y, bar.width, segment_height, red);
	draw_block(0, y + segment_height, bar.width, segment_height, green);
	draw_block(0, y + 2 * segment_height, bar.width, segment_height, red);
	bar.prev_y = y;
}

// Helper function to safely draw within bounds
void safe_draw_block(int x, int y, int w, int h, unsigned int color)
{
	// Clip coordinates and dimensions to screen bounds
	if (x < 0)
	{
		w += x; // Reduce width by the amount we're off-screen
		x = 0;
	}
	if (y < 0)
	{
		h += y; // Reduce height by the amount we're off-screen
		y = 0;
	}
	if (x + w > width)
	{
		w = width - x;
	}
	if (y + h > height)
	{
		h = height - y;
	}

	// Only draw if we have positive dimensions
	if (w > 0 && h > 0)
	{
		draw_block(x, y, w, h, color);
	}
}

void erase_ball()
{
	// Erase a square that's big enough to cover both the ball and its movement
	int erase_size = ball.diameter + ball.speed * 2; // Add speed*2 to cover movement in any direction
	safe_draw_block(ball.prev_x - ball.speed, ball.prev_y - ball.speed,erase_size, erase_size, white);
}

void draw_ball()
{
	erase_ball();

	// Save current position for next erase
	ball.prev_x = ball.x;
	ball.prev_y = ball.y;

	// First fill the entire ball area with white
	safe_draw_block(ball.x, ball.y, ball.diameter, ball.diameter, white);

	// Then draw the diamond shape in black
	if (ball.x >= 0 && ball.x < width - 7 && ball.y >= 0 && ball.y < height - 7)
	{
		draw_block(ball.x + 3, ball.y, 1, 1, black);
		draw_block(ball.x + 2, ball.y + 1, 3, 1, black);
		draw_block(ball.x + 1, ball.y + 2, 5, 1, black);
		draw_block(ball.x, ball.y + 3, 7, 1, black);
		draw_block(ball.x + 1, ball.y + 4, 5, 1, black);
		draw_block(ball.x + 2, ball.y + 5, 3, 1, black);
		draw_block(ball.x + 3, ball.y + 6, 1, 1, black);
	}
}

void draw_playing_field()
{
	// Draw blocks
	for (int i = 0; i < NROWS; i++)
	{
		for (int j = 0; j < NCOLS; j++)
		{
			if (!tiles[i][j])
			{ // if block not destroyed
				// Calculate x position starting from the right edge
				int x = width - ((NCOLS - j) * TILE_SIZE);
				int y = i * TILE_SIZE;

				unsigned int color;
				if ((i + j) % 2 == 0)
				{
					color = red;
				}
				else
				{
					color = blue;
				}
				draw_block(x, y, TILE_SIZE - 1, TILE_SIZE - 1, color);
			}
		}
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

void write(char *str)
{
	for (int i = 0; str[i] != '\0'; i++)
		WriteUart(str[i]);
}

#define BALL_SPEED 3

#define BAR_WIDTH 7
#define BAR_SEGMENT_HEIGHT 15
#define BAR_TOTAL_HEIGHT (BAR_SEGMENT_HEIGHT * 3)
#define BAR_STEP 10

// It must initialize the game
void reset()
{
	game_state = Stopped;

	ball.x = width / 2;
	ball.y = height / 2;
	ball.horizontal_direction = RIGHT;
	ball.vertical_direction = UP;
	ball.diameter = BALL_DIAMETER;
	ball.speed = BALL_SPEED;
	ball.prev_x = ball.x;
	ball.prev_y = ball.y;

	bar.x = 0;
	bar.y = height / 2;
	bar.width = BAR_WIDTH;
	bar.height = BAR_TOTAL_HEIGHT;
	bar.step = BAR_STEP;
	bar.prev_y = bar.y;

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
		switch (game_state)
		{
		case Stopped:
		{
			/* Start screen */
			write("Press 'w' or 's' to start game\n");
			while (game_state == Stopped)
			{
				unsigned long long out = ReadUart();
				if (!(out & 0x8000))
					continue;
				char c = out & 0xFF;
				switch (c)
				{
				case 'w':
				case 's':
					game_state = Running;
					break;

				case '\n':
				default:
					break;
				}
			}

			break;
		}
		case Running:
		{ /* Game running */

			int ball_right_edge = ball.x + ball.diameter;
			
			if (ball_right_edge <= LOSING_X_COORDINATE)
			{
				game_state = Lost;
				continue;
			}
			if (WINNING_X_COORDINATE < ball_right_edge)
			{
				game_state = Won;
				continue;
			}

			int ball_center_x = ball.x + (ball.diameter / 2);
			int ball_center_y = ball.y + (ball.diameter / 2);

			{ /* Physics */
				ball.x += ball.horizontal_direction * ball.speed;
				ball.y += ball.vertical_direction * ball.speed;

				// Bounce off top/bottom walls
				if (ball.y <= 0 || ball.y >= height - ball.diameter)
					ball.vertical_direction = (Vertical)(-ball.vertical_direction);

				// Check collision with bar using ball's leftmost edge for horizontal
				if (ball.x <= bar.x + bar.width && ball.x >= bar.x)
				{
					// Determine which region of the bar was hit
					int y_diff = ball_center_y - bar.y;
					int region_size = bar.height / 3;

					ball.horizontal_direction = RIGHT;
					
					if (y_diff < region_size)
					{
						ball.vertical_direction = UP;
					}
					else if (y_diff < 2 * region_size)
					{
						ball.vertical_direction = NEUTRAL;
					}
					else
					{
						ball.vertical_direction = DOWN;
					}
				}

				if (ball_right_edge >= width - (NCOLS * TILE_SIZE))
				{
					int block_row = ball_center_y / TILE_SIZE;
					// Calculate the relative x position from the right edge
					int relative_x = width - ball_right_edge;
					// Convert to column index (0 is rightmost column)
					int block_column = NCOLS - 1 - (relative_x / TILE_SIZE);

					if (block_row >= 0 && block_row < NROWS &&
							block_column >= 0 && block_column < NCOLS &&
							!tiles[block_row][block_column])
					{
						// Destroy block and paint it white immediately
						tiles[block_row][block_column] = 1;
						// Calculate the position and paint it white - use same formula as drawing
						int x = width - ((NCOLS - block_column) * TILE_SIZE);
						int y = block_row * TILE_SIZE;
						draw_block(x, y, TILE_SIZE - 1, TILE_SIZE - 1, white);

						// Reverse horizontal direction
						ball.horizontal_direction = (Horizontal)(-ball.horizontal_direction);

						// Check for corner hit with adjacent blocks
						if (ball.y % TILE_SIZE == 0)
						{
							if (block_row > 0 && !tiles[block_row - 1][block_column])
							{
								tiles[block_row - 1][block_column] = 1;
							}
							if (block_row < NROWS - 1 && !tiles[block_row + 1][block_column])
							{
								tiles[block_row + 1][block_column] = 1;
							}
						}
					}
				}
			}

			{ /* IO Update bar */
				unsigned long long out = ReadUart();
				if (out & 0x8000)
				{
					char c = out & 0xFF;
					if (c == 'w' && bar.y >= bar.step)
					{
						bar.y -= bar.step;
					}
					if (c == 's' && bar.y <= height - bar.height - bar.step)
					{
						bar.y += bar.step;
					}
				}
			}

			{ /* Update screen */
				// Draw new positions
				draw_playing_field();
				draw_ball();
				draw_bar(bar.y);
			}

			break;
		}
		case Won:
		{
			game_state = Stopped;
			write(won);
			reset();
			write("Try again?\n");

			break;
		}
		case Lost:
		{
			game_state = Stopped;
			write(lost);
			reset();
			write("Try again?\n");

			break;
		}
		case Exit:
		default:
			return 0;
		}
	}
	return 0;
}
