/*
 * collide.c
 * program which demonstrates sprites colliding with tiles
 */

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

/* include the tilemap image we are using */
#include "tilemap.h"

/* include the sprite image we are using */
#include "may.h"

/* include the tile map we are using */
//#include "map.h"
#include "darkness.h"
#include "maze.h"
#include "water.h"
#include "water2.h"
#include "title.h"
#include "title2.h"
/* the tile mode flags needed for display control register */
#define MODE0 0x00
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800
/* flags to set sprite handling in display control register */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000

/*define the timer control registers*/
//volatile unsigned short* timer0_data = (volatile unsigned short*) 0x4000100;
//volatile unsigned short* timer1_data = (volatile unsigned short*) 0x4000102;

/*bit postition for control register*/
//#define TIMER_FREQ_1 0x0

/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;
/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* there are 128 sprites on the GBA */
#define NUM_SPRITES 128

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the memory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* the address of the color palettes used for tilemaps and sprites */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* scrolling registers for backgrounds */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;
volatile short* bg2_x_scroll = (unsigned short*) 0x4000018;
volatile short* bg2_y_scroll = (unsigned short*) 0x400001a;
volatile short* bg3_x_scroll = (unsigned short*) 0x400001c;
volatile short* bg3_y_scroll = (unsigned short*) 0x400001e;

/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank() {
	/* wait until all 160 lines have been updated */
	while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
	/* and the button register with the button constant we want */
	unsigned short pressed = *buttons & button;

	/* if this value is zero, then it's not pressed */
	if (pressed == 0) {
		return 1;
	} else {
		return 0;
	}
}

/* return a pointer to one of the 4 character blocks (0-3) */
volatile unsigned short* char_block(unsigned long block) {
	/* they are each 16K big */
	return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

/* return a pointer to one of the 32 screen blocks (0-31) */
volatile unsigned short* screen_block(unsigned long block) {
	/* they are each 2K big */
	return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000
/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
	*dma_source = (unsigned int) source;
	*dma_destination = (unsigned int) dest;
	*dma_count = amount | DMA_16 | DMA_ENABLE;
}

/* function to setup tilemap 0 for this program */
void setup_tilemap() {

	/* load the palette from the image into palette memory*/
	memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) tilemap_palette, PALETTE_SIZE);

	/* load the image into char block 0 */
	memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) tilemap_data,
			(tilemap_width * tilemap_height) / 2);

	/* set all control the bits in this register */
	*bg0_control = 1 |    /* priority, 0 is highest, 3 is lowest */
		(0 << 2)  |       /* the char block the image data is stored in */
		(0 << 6)  |       /* the mosaic flag */
		(1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
		(16 << 8) |       /* the screen block the tile data is stored in */
		(1 << 13) |       /* wrapping flag */
		(3 << 14);        /* bg size, 0 is 256x256 */
	/* set all control the bits in this register */
	*bg1_control = 2 |    /* priority, 0 is highest, 3 is lowest */
		(0 << 2)  |       /* the char block the image data is stored in */
		(0 << 6)  |       /* the mosaic flag */
		(1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
		(21 << 8) |       /* the screen block the tile data is stored in */
		(1 << 13) |       /* wrapping flag */
		(0 << 14);        /* bg size, 0 is 256x256 */

	*bg3_control = 3 |    /* priority, 0 is highest, 3 is lowest */
		(0 << 2)  |       /* the char block the image data is stored in */
		(0 << 6)  |       /* the mosaic flag */
		(1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
		(23 << 8) |       /* the screen block the tile data is stored in */
		(1 << 13) |       /* wrapping flag */
		(0 << 14);        /* bg size, 0 is 256x256 */

	*bg2_control = 0 |    /* priority, 0 is highest, 3 is lowest */
		(0 << 2)  |       /* the char block the image data is stored in */
		(0 << 6)  |       /* the mosaic flag */
		(1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
		(24 << 8) |       /* the screen block the tile data is stored in */
		(1 << 13) |       /* wrapping flag */
		(0 << 14);	  /*bg size*/

	/* load the tile data into screen block 16 */
	//memcpy16_dma((unsigned short*) screen_block(16), (unsigned short*) maze, maze_width * maze_height);
	volatile unsigned short* dest = screen_block(0);
	dest = screen_block(16);
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 32; j++) {
			dest[i*32 + j] = maze[i*64 + j];
		}
	}
	dest = screen_block(17);
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 32; j++) {
			dest[i*32 + j] = maze[i*64+32 + j];
		}
	}
	dest = screen_block(18);
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 32; j++) {
			dest[i*32 + j] = maze[i*64 + j + 2048];
		}
	}
	dest = screen_block(19);
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 32; j++) {
			dest[i*32 + j] = maze[i*64+32 + j + 2048];
		}
	}
	dest = screen_block(21);
	//for (int i = 0; i < 32; i++) {
	for (int i = 0; i < (water_width * water_height); i++) {
		dest[i] = water[i];
	}
	dest = screen_block(22);
	//for (int i = 0; i < 32; i++) {
	for (int i = 0; i < (darkness_width * darkness_height); i++) {
		dest[i] = darkness[i];
	}
	dest = screen_block(23);
	//for (int i = 0; i < 32; i++) {
	for (int i = 0; i < (water2_width * water2_height); i++) {
		dest[i] = water2[i];
	}
	dest = screen_block(24);
        //for (int i = 0; i < 32; i++) {
        for (int i = 0; i < (title_width * title_height); i++) {
                dest[i] = title[i];
        }
	dest = screen_block(25);
        //for (int i = 0; i < 32; i++) {
        for (int i = 0; i < (title2_width * title2_height); i++) {
                dest[i] = title2[i];
        }
}

void ASSdel(int a);
/* just kill time */
void delay(unsigned int amount) {
	//for (int i = 0; i < amount * 10; i++);
	ASSdel(amount);
}

/* a sprite is a moveable image on the screen */
struct Sprite {
	unsigned short attribute0;
	unsigned short attribute1;
	unsigned short attribute2;
	unsigned short attribute3;
};

/* array of all the sprites available on the GBA */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* the different sizes of sprites which are possible */
enum SpriteSize {
	SIZE_8_8,
	SIZE_16_16,
	SIZE_32_32,
	SIZE_64_64,
	SIZE_16_8,
	SIZE_32_8,
	SIZE_32_16,
	SIZE_64_32,
	SIZE_8_16,
	SIZE_8_32,
	SIZE_16_32,
	SIZE_32_64
};

/* function to initialize a sprite with its properties, and return a pointer */
struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
		int horizontal_flip, int vertical_flip, int tile_index, int priority) {

	/* grab the next index */
	int index = next_sprite_index++;

	/* setup the bits used for each shape/size possible */
	int size_bits, shape_bits;
	switch (size) {
		case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
		case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
		case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
		case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
		case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
		case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
		case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
		case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
		case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
		case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
		case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
		case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
	}

	int h = horizontal_flip ? 1 : 0;
	int v = vertical_flip ? 1 : 0;

	/* set up the first attribute */
	sprites[index].attribute0 = y |             /* y coordinate */
		(0 << 8) |          /* rendering mode */
		(0 << 10) |         /* gfx mode */
		(0 << 12) |         /* mosaic */
		(1 << 13) |         /* color mode, 0:16, 1:256 */
		(shape_bits << 14); /* shape */

	/* set up the second attribute */
	sprites[index].attribute1 = x |             /* x coordinate */
		(0 << 9) |          /* affine flag */
		(h << 12) |         /* horizontal flip flag */
		(v << 13) |         /* vertical flip flag */
		(size_bits << 14);  /* size */

	/* setup the second attribute */
	sprites[index].attribute2 = tile_index |   // tile index */
		(priority << 10) | // priority */
		(0 << 12);         // palette bank (only 16 color)*/

	/* return pointer to this sprite */
	return &sprites[index];
}

/* update all of the spries on the screen */
void sprite_update_all() {
	/* copy them all over */
	memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
	/* clear the index counter */
	next_sprite_index = 0;

	/* move all sprites offscreen to hide them */
	for(int i = 0; i < NUM_SPRITES; i++) {
		sprites[i].attribute0 = SCREEN_HEIGHT;
		sprites[i].attribute1 = SCREEN_WIDTH;
	}
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
	/* clear out the y coordinate */
	sprite->attribute0 &= 0xff00;

	/* set the new y coordinate */
	sprite->attribute0 |= (y & 0xff);

	/* clear out the x coordinate */
	sprite->attribute1 &= 0xfe00;

	/* set the new x coordinate */
	sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
	/* get the current y coordinate */
	int y = sprite->attribute0 & 0xff;

	/* get the current x coordinate */
	int x = sprite->attribute1 & 0x1ff;

	/* move to the new location */
	sprite_position(sprite, x + dx, y + dy);
}

/* change the vertical flip flag */
void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
	if (vertical_flip) {
		/* set the bit */
		sprite->attribute1 |= 0x2000;
	} else {
		/* clear the bit */
		sprite->attribute1 &= 0xdfff;
	}
}

/* change the vertical flip flag */
void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
	if (horizontal_flip) {
		/* set the bit */
		sprite->attribute1 |= 0x1000;
	} else {
		/* clear the bit */
		sprite->attribute1 &= 0xefff;
	}
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
	/* clear the old offset */
	sprite->attribute2 &= 0xfc00;

	/* apply the new one */
	sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
	/* load the palette from the image into palette memory*/
	memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) may_palette, PALETTE_SIZE);

	/* load the image into char block 0 */
	memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) may_data, (may_width * may_height) / 2);
}

/* a struct for the may's logic and behavior */
struct May {
	/* the actual sprite attribute info */
	struct Sprite* sprite;

	/* the x and y postion, in 1/256 pixels */
	int x, y;

	/* which frame of the animation he is on */
	int frame;

	/* the number of frames to wait before flipping */
	int animation_delay;

	/* the animation counter counts how many frames until we flip */
	int counter;

	/*direction*/
	int dir;//up = 2, down = 0 left = -1 right = 1
};

/* initialize the may */
void may_init(struct May* may) {
	may->x = 112 << 8;
	may->y = 56 << 8;
	may->frame = 1;
	may->counter = 0;
	may->animation_delay = 8;
	may->dir = 0;
	may->sprite = sprite_init(may->x >> 8, may->y >> 8, SIZE_16_32, 0, 0, may->frame*16, 0);
}

/* stop the may from walking left/right */
void may_stop(struct May* may) {
	int dir = may->dir;
	if (dir<0){
		dir = dir * -1;
	}
	may->frame = (dir*3)+1;
	may->counter = 7;
	sprite_set_offset(may->sprite, may->frame*16);
}

/* finds which tile a screen coordinate maps to, taking scroll into account */
unsigned short tile_lookup(int x, int y, int xscroll, int yscroll,
		const unsigned short* tilemap, int tilemap_w, int tilemap_h) {

	/* adjust for the scroll */
	x += xscroll;
	y += yscroll;

	/* convert from screen coordinates to tile coordinates */
	x >>= 3;
	y >>= 3;

	/* account for wraparound */
	while (x >= tilemap_w) {
		x -= tilemap_w;
	}
	while (y >= tilemap_h) {
		y -= tilemap_h;
	}
	while (x < 0) {
		x += tilemap_w;
	}
	while (y < 0) {
		y += tilemap_h;
	}

	/* lookup this tile from the map */
	int index = y * tilemap_w + x;

	/* return the tile */
	return tilemap[index];
}


/* update the may */
void may_update(struct May* may, int* xscroll, int* yscroll) {
	/*look at the tile we are stadnign on?
	  unsigned short tiled = tile_lookup((may->x >> 8) + 8, (may->y >> 8) + 32, *xscroll,
	 *yscroll, maze, maze_width, maze_height);
	 unsigned short tileu = tile_lookup((may->x >> 8) + 8, (may->y >> 8) + 24, *xscroll,
	 *yscroll, maze, maze_width, maze_height);
	 unsigned short tilel = tile_lookup((may->x >> 8) + 0, (may->y >> 8) + 28, *xscroll,
	 *yscroll, maze, maze_width, maze_height);
	 unsigned short tiler = tile_lookup((may->x >> 8) + 16, (may->y >> 8) + 28, *xscroll,
	 *yscroll, maze, maze_width, maze_height);

	 if(tiled==63 || tiled ==  0x03f0){*yscroll--;}
	 else if(tileu==0x03f1 || tileu ==  0x03f0){*yscroll++;}
	 else if(tilel==0x03f1 || tilel ==  0x03f0){*xscroll++;}
	 else if(tiler==0x03f1 || tiler==  0x03f0){*xscroll--;}
	 */

	may->counter++;
	if (may->dir == -1){
		sprite_set_horizontal_flip(may->sprite,1);
		//sprite_set_horizontal_flip(second->sprite,1);
	}else{
		sprite_set_horizontal_flip(may->sprite,0);
		//sprite_set_horizontal_flip(second->sprite,0);
	}
	int dir = may->dir;
	if (dir<0){
		dir = dir * -1;
	}
	if(may->counter > may->animation_delay){
		may->frame = ((dir * 3) + ((may->frame+1)%3));
		//second->frame = ((dir*3)+((seond->frame+1)%3));
		sprite_set_offset(may->sprite,may->frame*16);
		//sprite_set_offset(second->sprite,may->frame*16);
		may->counter = 0;
	}
	sprite_position(may->sprite,may->x>>8,may->y>>8);
}

#include "brandon.c"//better than copy paste i dare say hahahahahaha // same as may basically
#include "sparkle.c"

int ASScnt(int* cnt);//adds 1 to cnt and returns 1 if %20 = true
/* the main function */
int main() {
	/* we set the mode to mode 0 with bg0 on */
	*display_control = MODE0 | BG0_ENABLE | BG2_ENABLE | BG3_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;

	/* setup the tilemap 0 */
	setup_tilemap();

	/* setup the sprite image data */
	setup_sprite_image();

	/* clear all the sprites on screen now */
	sprite_clear();

	/* create the may */
	struct May may;
	may_init(&may);

	struct Sparkle shine;
	sparkle_init(&shine);
	int isOn = 0;

	int SCORE = 0;
	//int differance = 0;
	//int buf = 128;
	struct Brandon second;
	brandon_init(&second);
	//second.sprite->attribute2 |= 1 << 10;
	//second.sprite = sprite_init(second.x >> 8, second.y >> 8, SIZE_16_32, 0, 0, second.frame*16, 1);
	/* set initial scroll to 0 */
	int xscroll = 156;
	int yscroll = 176;
	*bg2_x_scroll = 8;
	*bg2_y_scroll = -20;

	int checx,checy;

	int cnt = 0;
	unsigned short tiled,tileu,tilel,tiler;
	//int exp =0;
	/* loop forever */

	while (1){
		wait_vblank();
                *bg0_x_scroll = xscroll;
                *bg0_y_scroll = yscroll;
                *bg1_x_scroll = xscroll*2;
                *bg1_y_scroll = yscroll*2;
                *bg3_x_scroll = xscroll*2;
                *bg3_y_scroll = yscroll*2;
                //*display_control &= 0xfffff0ff;       
                //cnt++;
                //if(cnt%20==0){
                if (ASScnt(&cnt)){//increase cnt by 1, see if it can be mod 20
                        if(cnt%40==0){//BG1_ENABLE & *display_control == BG1_ENABLE){
                                *display_control &= 0xfffffdff;
                                *display_control |= BG3_ENABLE;
                        }else{
                                *display_control &= 0xfffff7ff;
                                *display_control |= BG1_ENABLE;
                        }
                        }
                        if(cnt==40){cnt=0;}
                        sprite_update_all();
                                /* delay some */
                                delay(300);
			xscroll++;
			yscroll--;
                 if  (button_pressed(BUTTON_START)) {
                	*display_control &=  ~BG2_ENABLE;
			*bg2_control = 0 |    /* priority, 0 is highest, 3 is lowest */
	                (0 << 2)  |       /* the char block the image data is stored in */
        	        (0 << 6)  |       /* the mosaic flag */
                	(1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
	                (22 << 8) |       /* the screen block the tile data is stored in */
        	        (1 << 13) |       /* wrapping flag */
                	(0 << 14);        /*bg size*/
			delay(10000);
			break;
		}

	}
        xscroll = 156;
        yscroll = 176;
        *bg2_x_scroll = 8;
        *bg2_y_scroll = 48;

	while (1) {
		/* update the may */
		may_update(&may, &xscroll, &yscroll);
		//may_update(&second, &xscroll, &yscroll);
		if(may.dir == 0){// that  is facinf downwards
			second.y = (56-16)<<8;//may.y - 5;
			second.sprite->attribute2 |= 1 << 10;
			may.sprite->attribute2 &= 0xfbff;
			second.x = 112<<8;
		}else if (may.dir == 2){ //face up
			second.y = (56+16)<<8;//may.y + 5;
			may.sprite->attribute2 |= 1 << 10;
			second.sprite->attribute2 &= 0xfbff;
			second.x = 112<<8;
		}else if (may.dir == -1){
			second.y= 56<<8;
			second.x= (112+16)<<8;
		}else{
			second.y=56<<8;
			second.x=(112-16)<<8;
		}
		brandon_update(&second, &xscroll, &yscroll);
		sparkle_update(&shine,&xscroll,&yscroll);
		//exp++;
		//if (exp >20){
		//	may.frame+= 1;
		//	sprite_set_offset(may.sprite,exp%9);
		//	if (may.frame == 8){
		//		may.frame =1;
		//	}
		//}
		if(isOn==0){
			checx = 102;
			checy = 100;
		}
		while(isOn == 0){
			unsigned short good = tile_lookup((may.x>>8)+checx,(may.y>>8)+checy,xscroll,
					yscroll,maze,maze_width,maze_height);
			if(good !=44){
				shine.x = ((may.x>>8)+checx) << 8;
				if (shine.x>512){
					shine.x=shine.x-512;
				}
				shine.y = ((may.y>>8)+checy) << 8;
				if (shine.y >256){
					shine.y=shine.y-256;
				}
				isOn = 1;
				break;
			}
			checx++;checy++;
		}	
		tiled = tile_lookup((may.x >> 8) + 8, (may.y >> 8) + 32, xscroll,
				yscroll, maze, maze_width, maze_height);
		tileu = tile_lookup((may.x >> 8) + 8, (may.y >> 8) + 24, xscroll,
				yscroll, maze, maze_width, maze_height);
		tilel = tile_lookup((may.x >> 8) + 0, (may.y >> 8) + 28, xscroll,
				yscroll, maze, maze_width, maze_height);
		tiler = tile_lookup((may.x >> 8) + 16, (may.y >> 8) + 28, xscroll,
				yscroll, maze, maze_width, maze_height);

		//if(tiled==63 || tiled ==  0x03f0){*yscroll--;}
		//else if(tileu==0x03f1 || tileu ==  0x03f0){*yscroll++;}
		//else if(tilel==0x03f1 || tilel ==  0x03f0){*xscroll++;}
		//else if(tiler==0x03f1 || tiler==  0x03f0){*xscroll--;}

		/* now the arrow keys move the may */
		if (button_pressed(BUTTON_RIGHT)) {
			// if (may_right(&may)) {
			may.dir = 1;
			second.dir = 1;
			if(tiler!= 44){
				if(button_pressed(BUTTON_B)){
					xscroll++;
					shine.x= ((shine.x>>8)-1<<8);
				}
				xscroll++;
				shine.x= ((shine.x>>8)-1<<8);
			}
			//}
		} else if (button_pressed(BUTTON_LEFT)) {
			//if (may_left(&may)) {
			may.dir = -1;
			second.dir = -1;
			if(tilel != 44){
				if(button_pressed(BUTTON_B)){
                                        xscroll--;
					shine.x= ((shine.x>>8)+1<<8);
                                }
				xscroll--;
				shine.x= ((shine.x>>8)+1<<8);
			}
		}else if (button_pressed(BUTTON_UP)){
			may.dir = 2;
			second.dir = 2;
			if(tileu != 44){
				if(button_pressed(BUTTON_B)){
                                        yscroll--;
					shine.y= ((shine.y>>8)+1<<8);
                                }
				yscroll--;
				shine.y= ((shine.y>>8)+1<<8);
			}
		}else if(button_pressed(BUTTON_DOWN)){
			may.dir= 0;
			second.dir = 0;
			if(tiled != 44){
				if(button_pressed(BUTTON_B)){
                                        yscroll++;
                                	shine.y= ((shine.y>>8)-1<<8);
				}
				yscroll++;
				shine.y= ((shine.y>>8)-1<<8);
			}
		}else if(button_pressed(BUTTON_A)){//||button_pressed(BUTTON_B)){	
			//if(shine.x > 512){shine.x=(shine.x= shine.x -512)<<8;}
			//else if (shine.x < 0){shine.x= shine.x+512;}

			//if(shine.y > 256){shine.y=shine.y-256;}
			//else if(shine.y < 0){shine.y=shine.y+256;}

			if((may.x >= shine.x-(16<<8) && may.x <= ((shine.x>>8)+16)<<8 && ((may.y>>8)+24)<<8 >= shine.y && ((may.y>>8)+24)<<8 <= ((shine.y>>8)+16)<<8) ||
					(may.x >= shine.x-(16<<8) && may.x <= ((shine.x>>8)+16)<<8 && ((may.y>>8)+32)<<8 >= shine.y && ((may.y>>8)+32)<<8 <= ((shine.y>>8)+16)<<8)||
					(may.x >= shine.x-(16<<8) && may.x <= ((shine.x>>8)+16)<<8 && ((may.y>>8)+16)<<8 >= shine.y && ((may.y>>8)+16)<<8 <= ((shine.y>>8)+16)<<8)){
				SCORE++;
				isOn = 0;
				*display_control |= BG2_ENABLE;
				//break;
			}
		}else if(button_pressed(BUTTON_START)){//pause
				*bg2_y_scroll = 15;
				*display_control |=  BG2_ENABLE;
	                        *bg2_control = 0 |    /* priority, 0 is highest, 3 is lowest */
        	                (0 << 2)  |       /* the char block the image data is stored in */
                	        (0 << 6)  |       /* the mosaic flag */
                        	(1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
	                        (25 << 8) |       /* the screen block the tile data is stored in */
        	                (1 << 13) |       /* wrapping flag */
                	        (0 << 14);        /*bg size*/
				delay(30000);
			while(1){
				if(button_pressed(BUTTON_START)){
					*bg2_y_scroll = 48;
					*bg2_control = 0 |    /* priority, 0 is highest, 3 is lowest */
	        	                (0 << 2)  |       /* the char block the image data is stored in */
        	        	        (0 << 6)  |       /* the mosaic flag */
                	        	(1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
                        		(22 << 8) |       /* the screen block the tile data is stored in */
		         		(1 << 13) |       /* wrapping flag */
	        	                (0 << 14);        /*bg size*/

					delay(10000);
					break;
				}
			}
		
		}else {//up = 2, down = 0 left = -1 right = 1
			int mmsx = may.x - shine.x;//how much to the right
	                int smmx = shine.x - may.x;//how much tp the left
        	        int mmsy = may.y - shine.y;//how much bellow
                	int smmy = shine.y - may.y;//how much above

			if(mmsx > mmsy && mmsx > smmx && mmsx > smmy){
				second.dir = -1;
			}else if(mmsy > mmsx && mmsy > smmx && mmsy > smmy){
				second.dir = 2;
			}else if(smmy > mmsx && smmy > smmx && smmy > mmsy){
                                second.dir = 0;
                        }else{
				second.dir = 1;//right
			}
			may_stop(&may);
			brandon_stop(&second);
		}

		/*
		int lasD = differance;
		int mmsx = may.x - shine.x;
		int smmx = shine.x - may.x;
		int mmsy = may.y - shine.y;
		int smmy = shine.y - may.y;
		if(mmsx > differance){differance=mmsx;}
		//if(smmx > differance){differance=smmx;}
		if(mmsy > differance){differance=mmsy;}
		//if(smmy > differance){differance=smmy;}

		if(differance > buf){	
			for(int counter = 0;counter<256;counter++){
				bg_palette[counter] = bg_palette[counter]-(differance - lasD);
			}
		}
		if((differance*-1) < (buf * -1)){
			for(int counter = 0;counter<256;counter++){
				bg_palette[counter] = bg_palette[counter]+(differance - lasD);
			}
		}*/

		/*
		   if((shine.x) > 512){shine.x-=512;}
		   if((shine.x) < 0){shine.x+=512;}

		   if((shine.y) = 256){shine.y-=256;}
		   if((shine.y) = 0){shine.y+=256;}
		//*/
		/* wait for vblank before scrolling and moving sprites */
		wait_vblank();
		*bg0_x_scroll = xscroll;
		*bg0_y_scroll = yscroll;
		*bg1_x_scroll = xscroll*2;
		*bg1_y_scroll = yscroll*2;
		*bg3_x_scroll = xscroll*2;
		*bg3_y_scroll = yscroll*2;
		//*display_control &= 0xfffff0ff;	
		//cnt++;
		//if(cnt%20==0){
		if (ASScnt(&cnt)){//increase cnt by 1, see if it can be mod 20
			if(cnt%40==0){//BG1_ENABLE & *display_control == BG1_ENABLE){
				*display_control &= 0xfffffdff;
				*display_control |= BG3_ENABLE;
			}else{
				*display_control &= 0xfffff7ff;
				*display_control |= BG1_ENABLE;		
			}
			}
			if(cnt==40){cnt=0;}
			sprite_update_all();
				/* delay some */
				delay(300);
		}
		}

		/* the game boy advance uses "interrupts" to handle certain situations
		 * for now we will ignore these */
		void interrupt_ignore() {
			/* do nothing */
		}

		/* this table specifies which interrupts we handle which way
		 * for now, we ignore all of them */
		typedef void (*intrp)();
		const intrp IntrTable[13] = {
			interrupt_ignore,   /* V Blank interrupt */
			interrupt_ignore,   /* H Blank interrupt */
			interrupt_ignore,   /* V Counter interrupt */
			interrupt_ignore,   /* Timer 0 interrupt */
			interrupt_ignore,   /* Timer 1 interrupt */
			interrupt_ignore,   /* Timer 2 interrupt */
			interrupt_ignore,   /* Timer 3 interrupt */
			interrupt_ignore,   /* Serial communication interrupt */
			interrupt_ignore,   /* DMA 0 interrupt */
			interrupt_ignore,   /* DMA 1 interrupt */
			interrupt_ignore,   /* DMA 2 interrupt */
			interrupt_ignore,   /* DMA 3 interrupt */
			interrupt_ignore,   /* Key interrupt */
		};

