/* a struct for the brandon's logic and behavior */
struct Brandon {
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

/* initialize the brandon */
void brandon_init(struct Brandon* brandon) {
	brandon->x = 112 << 8;
	brandon->y = 56 << 8;
	brandon->frame = 9;
	brandon->counter = 0;
	brandon->animation_delay = 8;
	brandon->dir = 0;
	brandon->sprite = sprite_init(brandon->x >> 8, brandon->y >> 8, SIZE_16_32, 0, 0, brandon->frame*16, 0);
}

/* stop the brandon from walking left/right */
void brandon_stop(struct Brandon* brandon) {
	int dir = brandon->dir;
	if (dir<0){
		dir = dir * -1;
	}
	brandon->frame = ((dir*3)+9)+1;
	brandon->counter = 7;
	sprite_set_offset(brandon->sprite, brandon->frame*16);
}

/* update the brandon */
void brandon_update(struct Brandon* brandon, int* xscroll, int* yscroll) {
	/*look at the tile we are stadnign on?
	  unsigned short tiled = tile_lookup((brandon->x >> 8) + 8, (brandon->y >> 8) + 32, *xscroll,
	 *yscroll, maze, maze_width, maze_height);
	 unsigned short tileu = tile_lookup((brandon->x >> 8) + 8, (brandon->y >> 8) + 24, *xscroll,
	 *yscroll, maze, maze_width, maze_height);
	 unsigned short tilel = tile_lookup((brandon->x >> 8) + 0, (brandon->y >> 8) + 28, *xscroll,
	 *yscroll, maze, maze_width, maze_height);
	 unsigned short tiler = tile_lookup((brandon->x >> 8) + 16, (brandon->y >> 8) + 28, *xscroll,
	 *yscroll, maze, maze_width, maze_height);

	 if(tiled==63 || tiled ==  0x03f0){*yscroll--;}
	 else if(tileu==0x03f1 || tileu ==  0x03f0){*yscroll++;}
	 else if(tilel==0x03f1 || tilel ==  0x03f0){*xscroll++;}
	 else if(tiler==0x03f1 || tiler==  0x03f0){*xscroll--;}
	 */

	brandon->counter++;
	if (brandon->dir == -1){
		sprite_set_horizontal_flip(brandon->sprite,1);
		//sprite_set_horizontal_flip(second->sprite,1);
	}else{
		sprite_set_horizontal_flip(brandon->sprite,0);
		//sprite_set_horizontal_flip(second->sprite,0);
	}
	int dir = brandon->dir;
	if (dir<0){
		dir = dir * -1;
	}
	if(brandon->counter > brandon->animation_delay){
		brandon->frame = ((dir * 3) + ((brandon->frame+1)%3)+9);
		//second->frame = ((dir*3)+((seond->frame+1)%3));
		sprite_set_offset(brandon->sprite,brandon->frame*16);
		//sprite_set_offset(second->sprite,brandon->frame*16);
		brandon->counter = 0;
	}
	sprite_position(brandon->sprite,brandon->x>>8,brandon->y>>8);
}
