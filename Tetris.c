/*
	 If you are playing this game in CPULATOR use can use keyboard arrows to play,
	 but in hardware u can only play using the push buttons
	 
	 In CPULATOR :-(U can use push buttons as well)
	 1) After compile load and continue (F5 followed by F3)
	 2)Click on the "Type here" in PS2/Keyboard or mouse option in cpulator at ff200100
	 3)Now right and left key board options can be used to by pressing to move the active piece left and right
	 4)We use up key for rotationg the 4 * 4 piece
	 5)By pressing and holding the down of keyboard the piece will fall down faster.
	 6)Click on push button 3 to restart the game 
	 7) After restarted click on any of the push buttons to continue the game again
	 
	 In Hardware :- 
	 1)0 button to move left
	 2)1 button to move right
	 3)2 button to rotate the piece
	 3)3 button to restart 
	 
	 Buttons work on falling edge so u need to press and remove the button to show the effect.
	 
	 
	 The pieces are architecutured with the legacy principles where each piece fit in 4 * 4 block 
	 and audio also features the legacy 8 bit tetris theme.
*/
/* For printing onto Terminal */
#include <stdio.h>
/* For using rand function to generate a random piece */
#include <stdlib.h>

#define CHAR_BUF_BASE 0xC9000000
#define VGA_BASE      0xC8000000
#define PS2_BASE      0xFF200100
#define KEY_BASE      0xFF200050  
#define HEX3_BASE   0xFF200020
#define PIXEL_BUF_CTRL_BASE 0xFF203020
#define GRID_W 10  /* Classic Tetris width */
#define GRID_H 20  /* Classic Tetris height */
#define TILE_SZ 10
#define OFF_X 110 /* Horizontal Offset */
#define OFF_Y 20  /* Vertical Offset */
	
/* COLOURS */
#define COL_BLACK 0x0000
#define COL_WHITE 0xFFFF
#define COL_GREY  0xCE79 
#define COL_CYAN   0x07FF /* I */
#define COL_BLUE   0x001F /* J */
#define COL_ORANGE 0xFD20 /* L */
#define COL_YELLOW 0xFFE0 /* O */
#define COL_GREEN  0x07E0 /* S */
#define COL_PURPLE 0xF81F /* T */
#define COL_RED    0xF800 /* Z */


#define AUDIO_BASE  0xFF203040
#define SAMPLE_RATE 48000
#define VOLUME      0x07FFFFFF 

#define Q 3000     
#define E 1500     
#define DQ (Q + E) 
#define H 6000   
	
#define low_g_sharp 1661
#define low_a 1760
#define low_b 1975
#define g_sharp 3322
	
#define a 3520
#define b 3951
#define c 4186
#define d 4699
#define e 5274
#define f 5588
#define g 6272
#define high_g_sharp 6645
#define high_a 7040


int tetris_freqs[] = {
	/* Rythm 1 */
    e, b, c, d, c, b, a, a, c, e, d, c, b, b, c, d, e, c, a, a,
	
	/* Rythm 2 */
    d, f, high_a, g, f, e, c, e, d, c, b, b, c, d, e, c, a, a,
	
	/* Repeat 1 & 2 */
	e, b, c, d, c, b, a, a, c, e, d, c, b, b, c, d, e, c, a, a,
    d, f, high_a, g, f, e, c, e, d, c, b, b, c, d, e, c, a, a,
	
	/* Bridge / Transition. Rythm */
	e, c, d, b, c, a, g_sharp, b,
    e, c, d, b, c, e, high_a, high_g_sharp
};
	
int tetris_durs[] = {
    Q, E, E, Q, E, E, Q, E, E, Q, E, E, Q, E, E, Q, Q, Q, Q, H,
    DQ, E, Q, E, E, DQ, E, Q, E, E, Q, E, E, Q, Q, Q, Q, H,
	
	Q, E, E, Q, E, E, Q, E, E, Q, E, E, Q, E, E, Q, Q, Q, Q, H,
    DQ, E, Q, E, E, DQ, E, Q, E, E, Q, E, E, Q, Q, Q, Q, H,
	
	H, H, H, H, H, H, H, H,
    H, H, H, H, Q, Q, H, H
};

int current_note = 0;
int sample_counter = 0;
int wave_phase = 0; 
int num_notes = sizeof(tetris_freqs) / sizeof(tetris_freqs[0]);

/* Prototyping */
void clear_char_buffer();
void clear_pixel_buffer();
void draw_pixel(int x, int y, short color);
void draw_grid();
void spawn_piece(int tag);
void draw_active_piece();
void draw_block(int grid_x, int grid_y, short color);
int is_valid_position(int new_x, int new_y, int test_shape[4][4]);
void erase_active_piece();
int move_piece(int dx, int dy);
void lock_piece();
void rotate_piece();
void delay();
int read_keys();
void redraw_board();
void clear_lines();
void print_string(int x, int y, char *text);
void reset_board();
void delay_char();
void play_bgm();
void smart_delay();
void init_audio();
void update_ps2();

const short PIECE_COLORS[7] = {
    COL_CYAN, COL_YELLOW, COL_PURPLE, COL_GREEN, COL_RED, COL_BLUE, COL_ORANGE
};

typedef struct {
    int tag;         /* 0 to 6 representing the type of piece */
    int x, y;        /* Position of the top-left corner of the 4x4 matrix on the grid */
    int shape[4][4]; /* The current layout of the piece */
} Piece;

const unsigned int tetris_shapes[7][4][4] = {
    /* 0: I */
    {{0, 0, 0, 0}, 
    {1, 1, 1, 1}, 
    {0, 0, 0, 0}, 
    {0, 0, 0, 0}},

    /* 1: O */
    {{1, 1, 0, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}},

    /* 2: T */ 
    {{0, 1, 0, 0},
    {1, 1, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}},

    /* 3: S */
    {{0, 1, 1, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}},

    /* 4: Z */
    {{1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}},

    /* 5: J */
    {{1, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}}, 

    /* 6: L */
    {{1, 1, 1, 0},
    {1, 0, 0, 0}, 
    {0, 0, 0, 0},
    {0, 0, 0, 0}}
};

Piece current_piece;
short board[GRID_H][GRID_W] = {0}; 

/* To add the key board features */
int key_left_pressed = 0;
int key_right_pressed = 0;
int key_down_pressed = 0;
int key_up_pressed = 0;

int main(void)
{
	int fall_timer;
    int is_locked;
    int game_over;
    int manual_restart; 
	int last_left, last_right, last_up;
	int keys;
	int pressing_left, pressing_right;
	int i, current_fall_threshold;
	/* Displays on JTAG UART */
	printf("Welcome to Tetris !\n");

    while (1)
	{
		/* */
        init_audio();
        clear_pixel_buffer();
        clear_char_buffer();
        reset_board();    
        draw_grid();     
        
        spawn_piece(rand() % 7); 
        draw_active_piece();
        
        fall_timer = 0;
        is_locked = 0;
        game_over = 0;
        manual_restart = 0; 
		last_left = 0, last_right = 0, last_up = 0;
        
        while (!game_over && !manual_restart)
		{
            if (is_locked)
			{
                clear_lines();            
                spawn_piece(rand() % 7);  
                
                if(!is_valid_position(current_piece.x, current_piece.y, current_piece.shape))
				{
                    game_over = 1; 
                    break; 
                }
                
                draw_active_piece();      
                is_locked = 0;            
                continue;                 
            }

            keys = read_keys();
			update_ps2();
			
			/* This part is for switches */
            if (keys != 0)
			{

                if (keys & 0x8)
				{
                    manual_restart = 1; 
                    break;             
                }
                
                if (keys & 0x4) rotate_piece();
                

                pressing_left = keys & 0x2;
                

                pressing_right = keys & 0x1;
                
                if (pressing_right && !pressing_left)
				{
                    move_piece(1, 0);
                }
				else if (pressing_left && !pressing_right)
				{
                    move_piece(-1, 0);
                }
            }
			
			if (key_up_pressed && !last_up)
			{
                rotate_piece();
            }
            if (key_right_pressed && !last_right)
			{
                move_piece(1, 0);
            } 
			else if (key_left_pressed && !last_left)
			{
                move_piece(-1, 0);
            }
            last_up = key_up_pressed;
            last_left = key_left_pressed;
            last_right = key_right_pressed;

            fall_timer++;
            current_fall_threshold = key_down_pressed ? 2 : 30;
            
			if (fall_timer >= current_fall_threshold)
			{
                fall_timer = 0;
                if (move_piece(0, 1) == 0)
				{
                    lock_piece();
                    is_locked = 1; 
                }
            }
            
            smart_delay();
        } 
        
        
       /*INSTANT RESTART 
         if (manual_restart) {
             continue;
         } */


        clear_pixel_buffer(); 
        clear_char_buffer();  
        
        print_string(35, 28, "GAME OVER!");
        print_string(23, 40, "PRESS KEY 0, 1, OR 2 TO PLAY AGAIN");
        
        
        read_keys(); 
        
        for (i = 0; i < 30; i++)
		{
            delay();
        }
        
        while (1)
		{
            keys = read_keys();
        
            if (keys & 0x7)
			{ 
                break; 
            }
        }
        
    }
	
    return 0;
}

void clear_char_buffer()
{
	int y,x;
    volatile char *char_buffer = (char *)CHAR_BUF_BASE;

    for (y = 0; y < 60; y++)
	{
        for (x = 0; x < 80; x++)
		{
            char_buffer[y * 128 + x] = ' ';
        }
    }
}
void clear_pixel_buffer()
{
	int y, x;
    volatile short *pixel_buffer = (short *)VGA_BASE;

    for (y = 0; y < 240; y++)
	{
        for (x = 0; x < 320; x++)
		{
            pixel_buffer[x + y*512] = COL_BLACK; 
        }
    }
}

void draw_pixel(int x, int y, short color)
{
	volatile short *pixel_buffer;
    if (x < 0 || x >= 320 || y < 0 || y >= 240)
	{
        return; 
    }
    pixel_buffer = (short *)VGA_BASE;
    pixel_buffer[x + y * 512] = color; 
}

void draw_grid()
{
	int y, x,px ,py ,i;
    for (y = 0;y <= GRID_H;y++)
	{
        for (x = 0;x <= GRID_W;x++)
		{
            px = OFF_X + x * TILE_SZ;
            py = OFF_Y + y * TILE_SZ;
            for (i = 0;i < TILE_SZ;i++)
			{
                if ((px + i) < (OFF_X + GRID_W * TILE_SZ))
				{
                    draw_pixel((px + i), py, COL_WHITE); 
                }
                if ((py + i) < (OFF_Y + GRID_H * TILE_SZ))
				{
                    draw_pixel(px, (py + i), COL_WHITE); 
                }
            }
        }
    }
}

void spawn_piece(int tag)
{
	int row, col;
    current_piece.tag = tag;
    current_piece.x = 3; 
    current_piece.y = 0; 

    for (row = 0; row < 4; row++)
	{
        for (col = 0; col < 4; col++)
		{
            current_piece.shape[row][col] = tetris_shapes[tag][row][col];
        }
    }
}

void draw_active_piece()
{
	int row, col ,grid_x ,grid_y;
    short color = PIECE_COLORS[current_piece.tag];

    for (row = 0;row < 4;row++)
	{
        for (col = 0;col < 4;col++)
		{
            if (current_piece.shape[row][col] == 1)
			{
                grid_x = current_piece.x + col;
                grid_y = current_piece.y + row;
                draw_block(grid_x, grid_y, color);
            }
        }
    }
}



void draw_block(int grid_x, int grid_y, short color)
{
	int px ,py ,x ,y;
    if (grid_x < 0 || grid_x >= GRID_W || grid_y < 0 || grid_y >= GRID_H) return;
    
    px = OFF_X + (grid_x * TILE_SZ) + 1;
    py = OFF_Y + (grid_y * TILE_SZ) + 1;

    for (y = 0; y < TILE_SZ - 1; y++)
	{
        for (x = 0; x < TILE_SZ - 1; x++)
		{
            draw_pixel(px + x, py + y, color);
        }
    }
}

int is_valid_position(int new_x, int new_y, int test_shape[4][4])
{
	int row ,col ,grid_x ,grid_y;
    for (row = 0; row < 4; row++)
	{
        for (col = 0; col < 4; col++)
		{
            if (test_shape[row][col] == 1)
			{
                grid_x = new_x + col;
                grid_y = new_y + row;
                if (grid_x < 0 || grid_x >= GRID_W || grid_y >= GRID_H)
				{
                    return 0; 
                }
                if (board[grid_y][grid_x] != 0)
				{
                    return 0;
                }
            }
        }
    }
    return 1; 
}

void erase_active_piece()
{
	int row, col, grid_x, grid_y;
    for (row = 0; row < 4; row++)
	{
        for (col = 0; col < 4; col++)
		{
            if (current_piece.shape[row][col] == 1)
			{
                grid_x = current_piece.x + col;
                grid_y = current_piece.y + row;
                draw_block(grid_x, grid_y, COL_BLACK); 
            }
        }
    }
}

int move_piece(int dx, int dy)
{
    if (is_valid_position(current_piece.x + dx, current_piece.y + dy, current_piece.shape))
	{
        erase_active_piece();            
        current_piece.x += dx;           
        current_piece.y += dy;           
        draw_active_piece();             
        return 1; /* Success */
    }
    return 0; /* Blocked */
}

void lock_piece()
{
	int row, col;
    short color = PIECE_COLORS[current_piece.tag];
    for (row = 0; row < 4; row++)
	{
        for (col = 0; col < 4; col++)
		{
            if(current_piece.shape[row][col] == 1)
			{
                board[current_piece.y + row][current_piece.x + col] = color;
            }
        }
    }
}
void redraw_board()
{
	int y, x;
    for (y = 0; y < GRID_H; y++)
	{
        for (x = 0; x < GRID_W; x++)
		{
            if (board[y][x] != 0)
			{
                draw_block(x, y, board[y][x]); 
            } 
			else
			{
                draw_block(x, y, COL_BLACK); 
            }
        }
    }
}

void rotate_piece()
{
    int temp_shape[4][4];
	int row, col;
    for (row = 0; row < 4; row++)
	{
        for (col = 0; col < 4; col++)
		{
            temp_shape[col][3 - row] = current_piece.shape[row][col];
        }
    }
    
    if (is_valid_position(current_piece.x, current_piece.y, temp_shape))
	{
        erase_active_piece(); 
        
        for (row = 0; row < 4; row++)
		{
            for (col = 0; col < 4; col++)
			{
                current_piece.shape[row][col] = temp_shape[row][col];
            }
        }
        
        draw_active_piece(); 
    }
}

void delay()
{
    /* int count = 3000000; */
    int count = 50000; 
    volatile int i; 
    for (i = 0; i < count; i++){}
}

void delay_char() {
    int count = 3000000;
    /* int count = 50000; */ 
    volatile int i; 
    for (i = 0; i < count; i++) {}
}

int read_keys()
{
    volatile int *KEY_ptr = (int *)KEY_BASE;
    volatile int *edge_capture_ptr = KEY_ptr + 3;
    
    int edge = *edge_capture_ptr;
    *edge_capture_ptr = edge; 
    return edge;
}

void update_ps2()
{
	unsigned char byte;int state;
    volatile int *PS2_ptr = (int *)PS2_BASE;
    int PS2_data, VALID;
    
    static int is_break = 0; 

    while (1) {
        PS2_data = *(PS2_ptr);
        VALID = PS2_data & 0x8000;
        
        if (!VALID)
		{
            break; 
        }
        byte = PS2_data & 0xFF;

        if(byte == 0xE0)
		{
           continue; 
        }
		else if(byte == 0xF0)
		{
           is_break = 1;
        }
		else
		{
            state = is_break ? 0 : 1;
            if (byte == 0x6B) key_left_pressed = state;       
            else if (byte == 0x74) key_right_pressed = state; 
            else if (byte == 0x72) key_down_pressed = state;  
			else if(byte == 0x75) key_up_pressed = state;
            is_break = 0; 
        }
    }
}

void clear_lines()
{
    int lines_cleared = 0;int y, x;
	int shift_y;int is_full;
    
    for (y = GRID_H - 1; y >= 0; y--)
	{
        is_full = 1;
        
        for (x = 0; x < GRID_W; x++)
		{
            if (board[y][x] == 0)
			{
                is_full = 0;
                break;
            }
        }
        
        if (is_full)
		{
            for (shift_y = y; shift_y > 0; shift_y--)
			{
                for (x = 0; x < GRID_W; x++)
				{
                    board[shift_y][x] = board[shift_y - 1][x];
                }
            }
            for (x = 0; x < GRID_W; x++)
			{
                board[0][x] = 0;
            }
            
            lines_cleared++;
            y++; 
        }
    }

    if (lines_cleared > 0)
	{
        redraw_board();
    }
}


void print_string(int x, int y, char *text)
{
    volatile char *char_buffer = (char *)CHAR_BUF_BASE;
    int offset = (y * 128) + x; 
    
    while (*text)
	{
        *(char_buffer + offset) = *text;
        text++;
        offset++;
    }
}

void reset_board()
{
	int x, y;
    for (y = 0; y < GRID_H; y++)
	{
        for (x = 0; x < GRID_W; x++)
		{
            board[y][x] = 0;
        }
    }
}

void init_audio()
{
    volatile int *audio_ptr = (int *)AUDIO_BASE;
    *(audio_ptr) = 0x0000000C; 
    *(audio_ptr) = 0x00000000; 
}

void play_bgm()
{
	unsigned int i;
	int freq, duration, phase_step;
	int wave_val;
	int is_rest, output_val;
    volatile unsigned int *audio_ptr = (unsigned int *)AUDIO_BASE;
    
    unsigned int fifospace = *(audio_ptr + 1);
    unsigned int wsrc = (fifospace & 0xFF000000) >> 24;
    unsigned int wslc = (fifospace & 0x00FF0000) >> 16;
    unsigned int space = (wsrc < wslc) ? wsrc : wslc;

    for (i = 0; i < space; i++)
	{
        
        if (current_note >= num_notes)
		{
            current_note = 0; 
        }

        freq = tetris_freqs[current_note];
        duration = tetris_durs[current_note];
        
        phase_step = (freq * 100000) / SAMPLE_RATE;

        if (wave_phase < 25000)
		{ 
            wave_val = VOLUME;
        } 
		else
		{
            wave_val = -VOLUME; 
        }

        is_rest = (sample_counter > (duration - (duration / 10)));
        output_val = is_rest ? 0 : wave_val;


        *(audio_ptr + 2) = output_val; 
        *(audio_ptr + 3) = output_val; 

        wave_phase += phase_step;
        if (wave_phase >= 100000)
		{
            wave_phase -= 100000;
        }

        sample_counter++;
        if (sample_counter >= duration)
		{
            sample_counter = 0;
            current_note++;
        }
    }
}

void smart_delay()
{
    int count = 10000; 
    /* int count = 7000; */
    volatile int i; 
    int audio_timer = 0;
    
    for (i = 0; i < count; i++)
	{
        audio_timer++;
        if (audio_timer >= 500)
		{
            play_bgm();
            audio_timer = 0; 
        }
    }
}
	
	