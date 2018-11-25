/* a struct for the sparkle's logic and behavior */
struct Sparkle {
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

/* initialize the sparkle */
void sparkle_init(struct Sparkle* sparkle) {
	sparkle->x = 0 << 8;
	sparkle->y = 0 << 8;
	sparkle->frame = 18;
	sparkle->counter = 0;
	sparkle->animation_delay = 8;
	//sparkle->dir = 0;
	sparkle->sprite = sprite_init(sparkle->x >> 8, sparkle->y >> 8, SIZE_16_16, 0, 0, sparkle->frame*16, 1);
}
void setCor(struct Sparkle* sparkle,int x, int y){
	sparkle->x = x;
	sparkle->y = y;
}
/* stop the sparkle from walking left/right
void sparkle_stop(struct Sparkle* sparkle) {
	int dir = sparkle->dir;
	if (dir<0){
		dir = dir * -1;
	}
	sparkle->frame = ((dir*3)+9)+1;
	sparkle->counter = 7;
	sprite_set_offset(sparkle->sprite, sparkle->frame*16);
}
*/
/* update the sparkle */
void sparkle_update(struct Sparkle* sparkle, int* xscroll, int* yscroll) {
	sparkle->counter++;
	if(sparkle->counter > sparkle->animation_delay){
		if (sparkle->dir == 0){
			sprite_set_horizontal_flip(sparkle->sprite,1);
			sparkle->dir=1;
		}else{
			sprite_set_horizontal_flip(sparkle->sprite,0);
                        sparkle->dir=0;
		}
		/*
		if(sparkle->dir==1){sparkle->frame = 19; sparkle->dir=0;}
		else{sparkle->dir=1;sparkle->frame=18;} *///((dir * 3) + ((sparkle->frame+1)%3)+9);
		//second->frame = ((dir*3)+((seond->frame+1)%3));
		sprite_set_offset(sparkle->sprite,sparkle->frame*16);
		//sprite_set_offset(second->sprite,sparkle->frame*16);
		sparkle->counter = 0;
	}
	sprite_position(sparkle->sprite,sparkle->x>>8,sparkle->y>>8);
}
