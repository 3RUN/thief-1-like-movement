
// include lib:
#include <acknex.h>
#include <default.c>
#include <windows.h>
#include <strio.c>

// allow us to handle pointers by hand:
#define PRAGMA_POINTER

// define game paths:
#define PRAGMA_PATH "data"

// default trace/move flags:
#define TRACE_FLAGS (IGNORE_ME | IGNORE_PASSABLE | IGNORE_PASSENTS | IGNORE_MAPS | IGNORE_SPRITES | IGNORE_CONTENT)
#define MOVE_FLAGS (IGNORE_ME | IGNORE_PASSABLE | IGNORE_PASSENTS | IGNORE_MAPS | IGNORE_SPRITES | IGNORE_CONTENT)
// collusion groups/push values:
#define PUSH_GROUP										2
#define PLAYER_GROUP									3
#define ENEMY_GROUP										4
#define DOOR_GROUP										5
#define SWITCH_ITEM_GROUP								6
#define OBSTACLE_GROUP									7
#define LEVEL_GROUP										8

// object types:
#define OBJ_TYPE										skill70
#define OBJ_NONE										0
#define OBJ_CHAR										1
#define OBJ_PLATFORM									2

// different movement types:
#define MOVE_TYPE										skill71
#define MOVE_ON_FOOT									0
#define MOVE_GRAB_LEDGE									1
#define MOVE_HUNG_ON_LEDGE								2
#define MOVE_CLIMB_LEDGE								3
#define MOVE_ON_PIPE									4
#define MOVE_ON_ROPE									5
#define MOVE_ON_TIGHTROPE								6
#define MOVE_GET_JUMP_OVER								7
#define MOVE_JUMP_OVER_UP								8
#define MOVE_JUMP_OVER_FORWARD							9
#define MOVE_GRAB_STEP									10
#define MOVE_CLIMB_STEP									11

// global velocity speed:
#define SPEED_X											skill80
#define SPEED_Y											skill81
#define SPEED_Z											skill82

// define test level's name:
STRING *level_test_str = "map01.wmb";
// region names:
STRING *reg_ledge_str = "ledge_reg";
STRING *reg_step_str = "step_reg";
STRING *reg_jump_over_str = "jump_over_reg";

// pointers:
ENTITY *HERO_ENT;

// camera stuff:
VECTOR CAM_DIR;											// used for calculating dot product (to check if player is looking at something or not)
// pipe stuff:
VECTOR PIPE_TOP_VEC, PIPE_BOTTOM_VEC;					// top and down limits on the walkable pipe

// game parameters:
var GAME_LOCK_MOUSE = 1;								// 1 - if we need to lock the mouse in game window, other ways - 0
var MOUSE_SPEED_FACTOR = 1.00;							// mouse speed factor, to be changed from the game options
// global parameters:
var GLOBAL_GRAVITY = 6;									// gravity force used for characters
var GLOBAL_SLOPE_FACTOR = 0.5;							// gravity on slopes, determines the max angle to climb
var GLOBAL_STEP_HEIGHT = 16;							// max step we can walk up on
var GLOBAL_MOVEMENT_SCALE = 1;							// used to scale the movement
// camera parameters:
var CAM_GAME_FOV = 90;									// camera arc parameter (FOV)
// player's stuff
var HERO_ALLOW_MOVE = 1;								// 1 - if player is allowed to move, other ways - 0
var HERO_ALLOW_ROTATE = 1;								// 1 - if we are allowed to rotate, other ways - 0
var JUMP_FROM_SOMETHING = 0;							// 1 - if player jumped away from something (ledge, pipe, rope), other ways - 0
var HERO_CLIMB_TILT_LIMIT = -40;						// limit to switch movement up/down on the rope/pipe/ladder
var HERO_FALL_TIGHTROPE = 0;							// 1 - if player falled down from the tightrope, other ways - 0
var HERO_TIGHTROPE_SIDE = 0;							// our currently leaning side on the tightrope
var HERO_ALLOW_TIGHTROPE_FALL = 0;						// 1 - if we are allowed to fall down from tightrope

// player's action:
action actHero(){
	VECTOR CAM_POS;
	vec_fill(&CAM_POS.x, 0);	
	camera->arc = CAM_GAME_FOV;
	my->skill1 = 192;
	
	
	VECTOR TRACE_TO_VEC, MIN_VEC, FLOOR_NORMAL_VEC, FLOOR_SPEED_VEC;
	vec_fill(&TRACE_TO_VEC.x, 0); vec_fill(&MIN_VEC.x, 0);
	vec_fill(&FLOOR_NORMAL_VEC.x, 0); vec_fill(&FLOOR_SPEED_VEC.x, 0);
	
	VECTOR FORCE, ABS_FORCE, DIST, ABS_DIST;
	vec_fill(&FORCE.x, 0); vec_fill(&ABS_FORCE.x, 0);
	vec_fill(&DIST.x, 0); vec_fill(&ABS_DIST.x, 0);
	
	VECTOR FOOT_ONE_VEC, FOOT_TWO_VEC, FOOT_DIFF_VEC;
	vec_fill(&FOOT_ONE_VEC.x, 0); vec_fill(&FOOT_TWO_VEC.x, 0);
	vec_fill(&FOOT_DIFF_VEC.x, 0);
	
	VECTOR LEDGE_REG_MIN, LEDGE_REG_MAX, LEDGE_TRACE_VEC, LEDGE_HIT_VEC, LEDGE_OFFSET;
	vec_fill(&LEDGE_REG_MIN.x, 0); vec_fill(&LEDGE_REG_MAX.x, 0);
	vec_fill(&LEDGE_TRACE_VEC.x, 0); vec_fill(&LEDGE_HIT_VEC.x, 0);
	vec_fill(&LEDGE_OFFSET.x, 0);
	
	VECTOR FORCE_PIPE, DIST_PIPE;
	vec_fill(&FORCE_PIPE.x, 0); vec_fill(&DIST_PIPE.x, 0);
	
	VECTOR FORCE_ROPE, DIST_ROPE;
	vec_fill(&FORCE_ROPE.x, 0); vec_fill(&DIST_ROPE.x, 0);
	
	VECTOR FORCE_TIGHTROPE, DIST_TIGHTROPE;
	vec_fill(&FORCE_TIGHTROPE.x, 0); vec_fill(&DIST_TIGHTROPE.x, 0);
	
	VECTOR HANDRAIL_REG_MIN, HANDRAIL_REG_MAX;
	vec_fill(&HANDRAIL_REG_MIN.x, 0); vec_fill(&HANDRAIL_REG_MAX.x, 0);
	
	VECTOR STEP_REG_MIN, STEP_REG_MAX;
	vec_fill(&STEP_REG_MIN.x, 0); vec_fill(&STEP_REG_MAX.x, 0);
	
	ANGLE LEDGE_NORMAL_ANG;
	vec_fill(&LEDGE_NORMAL_ANG.pan, 0);
	
	var HEIGHT = 0, GRAVITY_TRACE_LENGTH = 96;
	var MOVE_SPEED = 5, MOVE_DIST = 0, MOVING_SPEED = 0, IS_MOVING = 0;
	var FRICTION = 0, AIR_FRICTION = 0.03, GND_FRICTION = 0.5, LEDGE_FRICTION = 0.8;
	var JUMP_TARGET = 0, JUMP_HEIGHT = 45, JUMP_KEY_OFF = 1;
	var FOOT_HIT_X = 0, FOOT_HIT_Y = 0, FOOT_HIT_MIDDLE = 0;
	var FOOT_PUSH_FACTOR = 1, FOOT_TRACE_LENGTH = 16, FOOT_RADIUS = 16, FOOT_EDGE = 24, FOOT_DISABLE = 0;
	var LEDGE_TOTAL = 0, LEDGE_ALLOW_DETECT = 1, LEDGE_Z_HEIGHT = 0, LEDGE_TARGET_Z = 0, LEDGE_ALLOW_WALK = 1, LEDGE_JUMP_ANG = 90;
	var LEDGE_SPEED = 3, LEDGE_INPUT = 0, LEDGE_VEL = 0, LEDGE_DIST = 0, LEDGE_CAM_INPUT = 0, LEDGE_LEFT_OUT = 0, LEDGE_RIGHT_OUT = 0;
	var LEDGE_MOVE_WAVE = 0, LEDGE_MOVE_RATE = 25, LEDGE_MOVE_AMPL = 1;
	var PIPE_SPEED = 5, PIPE_MOVE_WAVE = 0, PIPE_MOVE_RATE = 35, PIPE_MOVE_AMPL = 1;
	var ROPE_SPEED = 5, ROPE_MOVE_WAVE = 0, ROPE_MOVE_RATE = 35, ROPE_MOVE_AMPL = 1;
	var TIGHTROPE_SPEED = 5;
	var HANDRAIL_TOTAL = 0, HANDRAIL_Z_HEIGHT = 0, HANDRAIL_TARGET_Z = 0;
	var CLIMB_STEP_TOTAL = 0, STEP_TARGET_Z = 0, STEP_Z_HEIGHT = 0;
	
	HERO_ENT = my;
	set(my, TRANSLUCENT);
	
	vec_fill(&my->scale_x, 1);
	wait(1);
	my->eflags |= FAT | NARROW;
	vec_for_min(&MIN_VEC, my);	
	vec_set(&my->max_x, vector(16, 16, 32));
	vec_set(&my->min_x, vector(-16, -16, -30));
	
	my->OBJ_TYPE = OBJ_CHAR;
	my->MOVE_TYPE = MOVE_ON_FOOT;
	
	my->group = PLAYER_GROUP;
	my->push = PLAYER_GROUP;
	
	while(my){
		
		if(HERO_ALLOW_MOVE == 1){
			
			// moving on foot (walk/running/crawling)
			if(my->MOVE_TYPE == MOVE_ON_FOOT){
				
				vec_set(&TRACE_TO_VEC, &my->x);
				TRACE_TO_VEC.z -= GRAVITY_TRACE_LENGTH;			
				c_ignore(PLAYER_GROUP, 0);
				my->min_z *= 0.1; my->max_z *= 0.1;
				c_ignore(PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);			
				c_trace(&my->x, &TRACE_TO_VEC, TRACE_FLAGS | IGNORE_PUSH | IGNORE_FLAG2 | SCAN_TEXTURE | USE_BOX);
				my->min_z = -30; my->max_z = 32;
				HEIGHT = -999;
				if(HIT_TARGET){
					HEIGHT = my->z + MIN_VEC.z - target.z;
					vec_set(&FLOOR_NORMAL_VEC, &normal.x);
					vec_fill(&FLOOR_SPEED_VEC, 0);				
					// if the player is standing on a platform, move him with it
					if(you != NULL){
						if(you->OBJ_TYPE == OBJ_PLATFORM){
							FLOOR_SPEED_VEC.x = you->SPEED_X;
							FLOOR_SPEED_VEC.y = you->SPEED_Y;
							// Z speed is not necessary - this is done by the height adaption
						}
					}
				}
				
				// input from player:
				FORCE.x = MOVE_SPEED * (key_w - key_s);
				FORCE.y = MOVE_SPEED * (key_a - key_d);
				if(key_space){
					if(JUMP_KEY_OFF == 1){ FORCE.z = 1; JUMP_KEY_OFF = 0; }
				}
				else{ JUMP_KEY_OFF = 1; }
				
				if(vec_length(vector(FORCE.x, FORCE.y, 0)) > MOVE_SPEED){					
					var len = sqrt(FORCE.x * FORCE.x + FORCE.y * FORCE.y);					
					FORCE.x *= ((MOVE_SPEED) / len); FORCE.y *= ((MOVE_SPEED) / len);
				}
				
				if(key_shift){ FORCE.x *= 2; FORCE.y *= 2; }
				
				if(HEIGHT < 5 && HEIGHT != -999){
					FRICTION = GND_FRICTION;						
					vec_fill(&ABS_FORCE.x, 0);
					if(FLOOR_NORMAL_VEC.z < 0.9){ FORCE.x *= FLOOR_NORMAL_VEC.z; FORCE.y *= FLOOR_NORMAL_VEC.z; }
					if(FOOT_DISABLE == 1){ FOOT_DISABLE = 0; }
				}
				else{
					FRICTION = AIR_FRICTION;
					vec_scale(&FORCE.x, 0.1);
					FORCE.z = 0;
					vec_set(&ABS_FORCE.x, vector(0, 0, -GLOBAL_GRAVITY));
				}
				
				if(JUMP_FROM_SOMETHING == 1){
					FORCE.x = MOVE_SPEED;
					FORCE.x *= 15;
					JUMP_TARGET = JUMP_HEIGHT;
					JUMP_FROM_SOMETHING = 0;
				}
				
				if(HERO_FALL_TIGHTROPE == 1){
					FORCE.y = MOVE_SPEED * -HERO_TIGHTROPE_SIDE;
					FORCE.y *= 2;
					HERO_FALL_TIGHTROPE = 0;
				}
				vec_rotate(&FORCE.x, vector(camera->pan, 0, 0));
				
				vec_set(&FOOT_ONE_VEC, &my->x);
				FOOT_ONE_VEC.x += sign(FORCE.x) + (FOOT_TRACE_LENGTH * sign(FORCE.x));				
				my->min_x *= 0.1; my->max_x *= 0.1;
				c_ignore(PUSH_GROUP, PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);			
				FOOT_HIT_X = c_trace(&my->x, &FOOT_ONE_VEC.x, TRACE_FLAGS | IGNORE_FLAG2 | USE_BOX);
				my->min_x = -16; my->max_x = 16;				
				if(HIT_TARGET && abs(normal.z) < GLOBAL_SLOPE_FACTOR && (hit->z - (my->z + my->min_z)) > GLOBAL_STEP_HEIGHT){
					if(vec_dist(&my->x, &hit->x) < FOOT_RADIUS){						
						if(hit->x < my->x){ FORCE.x = maxv(FORCE.x, 0); }
						else{ FORCE.x = minv(FORCE.x, 0); }
						if(HEIGHT > 5){
							FORCE.x += normal.x * (FOOT_PUSH_FACTOR - vec_dot(&normal.x, &FORCE.x));
							FORCE.y += normal.y * (FOOT_PUSH_FACTOR - vec_dot(&normal.x, &FORCE.x));
							DIST.x = my->SPEED_X = FORCE.x;
							DIST.y = my->SPEED_Y = FORCE.y;
						}
					}
				}
				vec_set(&FOOT_TWO_VEC.x, &my->x);
				FOOT_TWO_VEC.y += sign(FORCE.y) + (FOOT_TRACE_LENGTH * sign(FORCE.y));				
				my->min_y *= 0.1; my->max_y *= 0.1;
				c_ignore(PUSH_GROUP, PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);			
				FOOT_HIT_Y = c_trace(&my->x, &FOOT_TWO_VEC.x, TRACE_FLAGS | IGNORE_FLAG2 | USE_BOX);
				my->min_y = -16; my->max_y = 16;				
				if(HIT_TARGET && abs(normal.z) < GLOBAL_SLOPE_FACTOR && (hit->z - (my->z + my->min_z)) > GLOBAL_STEP_HEIGHT){
					if(vec_dist(&my->x, &hit->x) < FOOT_RADIUS){						
						if(hit->y < my->y){ FORCE.y = maxv(FORCE.y, 0); }
						else{ FORCE.y = minv(FORCE.y, 0); }						
						if(HEIGHT > 5){
							FORCE.x += normal.x * (FOOT_PUSH_FACTOR - vec_dot(&normal.x, &FORCE.x));
							FORCE.y += normal.y * (FOOT_PUSH_FACTOR - vec_dot(&normal.x, &FORCE.x));
							DIST.x = my->SPEED_X = FORCE.x;
							DIST.y = my->SPEED_Y = FORCE.y;
						}
					}
				}
				vec_diff(&FOOT_DIFF_VEC, &FOOT_ONE_VEC.x, vec_inverse(&FOOT_TWO_VEC.x));
				vec_sub(&FOOT_DIFF_VEC.x, &my->x);				
				c_ignore(PUSH_GROUP, PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);			
				FOOT_HIT_MIDDLE = c_trace(&my->x, &FOOT_DIFF_VEC.x, TRACE_FLAGS | IGNORE_FLAG2 | USE_BOX);				
				if(HIT_TARGET && abs(normal.z) < GLOBAL_SLOPE_FACTOR && (hit->z - (my->z + my->min_z)) > GLOBAL_STEP_HEIGHT){
					if(vec_dist(&my->x, &hit->x) < FOOT_EDGE && HEIGHT > 5 && FOOT_DISABLE == 0){
						FORCE.x += normal.x * (FOOT_PUSH_FACTOR - vec_dot(&normal.x, &FORCE.x));
						FORCE.y += normal.y * (FOOT_PUSH_FACTOR - vec_dot(&normal.x, &FORCE.x));
						DIST.x = my->SPEED_X = FORCE.x;
						DIST.y = my->SPEED_Y = FORCE.y;
					}
				}
				
				var FOOT_FRIC = maxv((1 - time_step * FRICTION), 0);
				my->SPEED_X = (time_step * FORCE.x) + (FOOT_FRIC * my->SPEED_X);
				my->SPEED_Y = (time_step * FORCE.y) + (FOOT_FRIC * my->SPEED_Y);
				my->SPEED_Z = (time_step * ABS_FORCE.z) + (FOOT_FRIC * my->SPEED_Z);
				
				DEBUG_VAR(my->SPEED_Z, 200);
				
				DIST.x = my->SPEED_X * time_step;
				DIST.y = my->SPEED_Y * time_step;
				DIST.z = 0;
				
				ABS_DIST.x = ABS_FORCE.x * time_step;
				ABS_DIST.y = ABS_FORCE.y * time_step;
				ABS_DIST.z = my->SPEED_Z * time_step;
				
				if(HEIGHT < 5 && HEIGHT != -999){
					if(FLOOR_NORMAL_VEC.z > GLOBAL_SLOPE_FACTOR / 4){
						ABS_DIST.z = -maxv(HEIGHT, -10 * time_step);
						if((HEIGHT + ABS_DIST.z) > 6){ ABS_DIST.z = -HEIGHT -6; }
					}
					if(FORCE.z > 0){ JUMP_TARGET = JUMP_HEIGHT - HEIGHT; }
					ABS_DIST.x += FLOOR_SPEED_VEC.x;
					ABS_DIST.y += FLOOR_SPEED_VEC.y;
				}
				
				if(JUMP_TARGET > 0){
					my->SPEED_Z = sqrt((JUMP_TARGET) * 2 * GLOBAL_GRAVITY);
					ABS_DIST.z = my->SPEED_Z * time_step * GLOBAL_MOVEMENT_SCALE;
					JUMP_TARGET -= ABS_DIST.z;
				}
				
				vec_scale(&DIST, GLOBAL_MOVEMENT_SCALE);
				
				move_friction = 0;
				move_min_z = 0.3;			
				c_ignore(PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);			
				MOVE_DIST = c_move(my, nullvector, &DIST.x, MOVE_FLAGS | IGNORE_PUSH | ACTIVATE_SHOOT | GLIDE);				
				if(MOVE_DIST > 0){ MOVING_SPEED = MOVE_DIST / time_step; IS_MOVING = 1; }
				else{ MOVING_SPEED = 0; IS_MOVING = 0; }
				
				move_friction = 0;
				move_min_z = -1;			
				c_ignore(PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);			
				c_move(my, nullvector, &ABS_DIST.x, MOVE_FLAGS | IGNORE_PUSH | IGNORE_FLAG2);
				if(HIT_TARGET){ FORCE.z = 0; JUMP_TARGET = 0; ABS_DIST.z = 0; my->SPEED_Z = 0; }
				
				// reset all previous movement on ledge:
				LEDGE_INPUT = LEDGE_VEL = LEDGE_DIST = LEDGE_CAM_INPUT = 0;
				HERO_ALLOW_TIGHTROPE_FALL = 0;
				HERO_TIGHTROPE_SIDE -= (HERO_TIGHTROPE_SIDE - 0) * time_step;
				vec_fill(&FORCE_PIPE.x, 0); vec_fill(&FORCE_PIPE.x, 0);
				vec_fill(&FORCE_ROPE.x, 0); vec_fill(&DIST_ROPE.x, 0);
				vec_fill(&FORCE_TIGHTROPE.x, 0); vec_fill(&DIST_TIGHTROPE.x, 0);
				
				// check for ledge only if we are falling down:
				if(ABS_DIST.z < 0 && (HEIGHT > 5 || HEIGHT == -999)){
					LEDGE_TOTAL = 0;
					while(region_get(reg_ledge_str, LEDGE_TOTAL, &LEDGE_REG_MIN, &LEDGE_REG_MAX) != 0 && LEDGE_ALLOW_DETECT == 1){
						LEDGE_TOTAL += 1;
						
						if(my->x - my->min_x < LEDGE_REG_MIN.x || my->x - my->max_x > LEDGE_REG_MAX.x){ continue; }
						if(my->y - my->min_y < LEDGE_REG_MIN.y || my->y - my->max_y > LEDGE_REG_MAX.y){ continue; }
						if(my->z + 16 < LEDGE_REG_MIN.z || my->z + 16 > LEDGE_REG_MAX.z){ continue; }
						
						LEDGE_Z_HEIGHT = LEDGE_REG_MAX.z - my->z;
						LEDGE_TARGET_Z = my->z + LEDGE_Z_HEIGHT;
						
						VECTOR LEFT_TRACE, RIGHT_TRACE;
						vec_set(&LEFT_TRACE.x, vector(44, 32, 24));
						vec_rotate(&LEFT_TRACE.x, vector(my->pan, 0, 0));
						vec_add(&LEFT_TRACE.x, &my->x);
						vec_set(&RIGHT_TRACE.x, vector(44, -32, 24));
						vec_rotate(&RIGHT_TRACE.x, vector(my->pan, 0, 0));
						vec_add(&RIGHT_TRACE.x, &my->x);
						LEDGE_LEFT_OUT = c_trace(&my->x, &LEFT_TRACE.x, TRACE_FLAGS | IGNORE_MODELS);
						LEDGE_RIGHT_OUT = c_trace(&my->x, &RIGHT_TRACE.x, TRACE_FLAGS | IGNORE_MODELS);
						if(LEDGE_LEFT_OUT != 0 && LEDGE_RIGHT_OUT != 0){
							vec_set(&LEDGE_TRACE_VEC.x, vector(32, 0, 16));
							vec_rotate(&LEDGE_TRACE_VEC.x, vector(my->pan, 0, 0));
							vec_add(&LEDGE_TRACE_VEC.x, &my->x);
							c_ignore(PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);
							c_trace(vector(my->x, my->y, my->z + 16), &LEDGE_TRACE_VEC.x, TRACE_FLAGS | IGNORE_MODELS | SCAN_TEXTURE | USE_BOX);
							if(HIT_TARGET){
								if(tex_flag3){ LEDGE_ALLOW_WALK = 0; }
								if(tex_flag2){
									vec_set(&LEDGE_HIT_VEC, &target.x);
									vec_to_angle(&LEDGE_NORMAL_ANG.pan, &normal.x);
									LEDGE_ALLOW_DETECT = 0;
									HERO_ALLOW_ROTATE = 0;
									my->MOVE_TYPE = MOVE_GRAB_LEDGE;
								}
							}
						}
					}
				}
				else{
					CLIMB_STEP_TOTAL = 0;
					if(IS_MOVING == 1 && key_w && key_space){
						while(region_get(reg_step_str, CLIMB_STEP_TOTAL, &STEP_REG_MIN, &STEP_REG_MAX) != 0 && LEDGE_ALLOW_DETECT == 1){
							CLIMB_STEP_TOTAL += 1;
							
							if(my->x - my->min_x < STEP_REG_MIN.x || my->x - my->max_x > STEP_REG_MAX.x){ continue; }
							if(my->y - my->min_y < STEP_REG_MIN.y || my->y - my->max_y > STEP_REG_MAX.y){ continue; }
							if(my->z < STEP_REG_MIN.z || my->z > STEP_REG_MAX.z){ continue; }
							
							STEP_Z_HEIGHT = STEP_REG_MAX.z - my->z;
							STEP_TARGET_Z = my->z + STEP_Z_HEIGHT;
							
							VECTOR LEFT_TRACE, RIGHT_TRACE;
							vec_set(&LEFT_TRACE.x, vector(44, 32, 24));
							vec_rotate(&LEFT_TRACE.x, vector(my->pan, 0, 0));
							vec_add(&LEFT_TRACE.x, &my->x);
							vec_set(&RIGHT_TRACE.x, vector(44, -32, 24));
							vec_rotate(&RIGHT_TRACE.x, vector(my->pan, 0, 0));
							vec_add(&RIGHT_TRACE.x, &my->x);
							LEDGE_LEFT_OUT = c_trace(&my->x, &LEFT_TRACE.x, TRACE_FLAGS | IGNORE_MODELS);
							LEDGE_RIGHT_OUT = c_trace(&my->x, &RIGHT_TRACE.x, TRACE_FLAGS | IGNORE_MODELS);
							if(LEDGE_LEFT_OUT != 0 && LEDGE_RIGHT_OUT != 0){
								vec_set(&LEDGE_TRACE_VEC.x, vector(32, 0, 16));
								vec_rotate(&LEDGE_TRACE_VEC.x, vector(my->pan, 0, 0));
								vec_add(&LEDGE_TRACE_VEC.x, &my->x);
								c_ignore(PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);
								c_trace(vector(my->x, my->y, my->z + 16), &LEDGE_TRACE_VEC.x, TRACE_FLAGS | IGNORE_MODELS | SCAN_TEXTURE | USE_BOX);
								if(HIT_TARGET){
									vec_set(&LEDGE_HIT_VEC, &target.x);
									vec_to_angle(&LEDGE_NORMAL_ANG.pan, &normal.x);
									LEDGE_ALLOW_DETECT = 0;
									HERO_ALLOW_ROTATE = 0;
									my->MOVE_TYPE = MOVE_GRAB_STEP;
									
								}
							}
						}
					}
				}
				
				HANDRAIL_TOTAL = 0;
				while(region_get(reg_jump_over_str, HANDRAIL_TOTAL, &HANDRAIL_REG_MIN, &HANDRAIL_REG_MAX) != 0){
					HANDRAIL_TOTAL += 1;
					
					if(my->x - my->min_x < HANDRAIL_REG_MIN.x || my->x - my->max_x > HANDRAIL_REG_MAX.x){ continue; }
					if(my->y - my->min_y < HANDRAIL_REG_MIN.y || my->y - my->max_y > HANDRAIL_REG_MAX.y){ continue; }
					
					HANDRAIL_Z_HEIGHT = HANDRAIL_REG_MAX.z - my->z;
					HANDRAIL_TARGET_Z = my->z + HANDRAIL_Z_HEIGHT;
				}
			}
			
			// lerp to the ledge position:
			if(my->MOVE_TYPE == MOVE_GRAB_LEDGE){
				// reset all previous movement on foot:
				vec_fill(&FORCE.x, 0); vec_fill(&ABS_FORCE.x, 0);
				vec_fill(&DIST.x, 0); vec_fill(&ABS_DIST.x, 0);
				vec_fill(&my->SPEED_X, 0);
				JUMP_TARGET = 0;
				MOVE_DIST = MOVING_SPEED = IS_MOVING = 0;
				// reset pushing from wall force:
				vec_fill(&FOOT_ONE_VEC.x, 0); vec_fill(&FOOT_TWO_VEC.x, 0);
				vec_fill(&FOOT_DIFF_VEC.x, 0);
				FOOT_HIT_X = FOOT_HIT_Y = FOOT_HIT_MIDDLE = 0;			
				// reset all previous movement on ledge:
				LEDGE_INPUT = LEDGE_VEL = LEDGE_DIST = LEDGE_CAM_INPUT = 0;
				HERO_ALLOW_TIGHTROPE_FALL = 0;
				HERO_TIGHTROPE_SIDE -= (HERO_TIGHTROPE_SIDE - 0) * time_step;
				vec_fill(&FORCE_PIPE.x, 0); vec_fill(&FORCE_PIPE.x, 0);
				vec_fill(&FORCE_ROPE.x, 0); vec_fill(&DIST_ROPE.x, 0);
				vec_fill(&FORCE_TIGHTROPE.x, 0); vec_fill(&DIST_TIGHTROPE.x, 0);
				
				VECTOR TEMP_VEC;
				vec_set(&TEMP_VEC.x, vector(64, 0, 16));
				vec_rotate(&TEMP_VEC.x, vector(my->pan, 0, 0));
				vec_add(&TEMP_VEC.x, &my->x);
				c_trace(vector(my->x, my->y, my->z + 16), &TEMP_VEC.x, TRACE_FLAGS | IGNORE_YOU | IGNORE_MODELS | SCAN_TEXTURE | USE_BOX);
				if(HIT_TARGET){
					if(tex_flag2){ vec_set(&LEDGE_HIT_VEC, &target.x); vec_to_angle(&LEDGE_NORMAL_ANG.pan, &normal.x); }
				}				
				camera->pan -= ang(camera->pan - ang(LEDGE_NORMAL_ANG.pan - 180)) * 0.5 * time_step;
				camera->tilt -= ang(camera->tilt - 25) * 0.5 * time_step;
				my->pan = camera->pan;
				
				vec_set(&LEDGE_OFFSET.x, vector(16, 0, 0));
				vec_rotate(&LEDGE_OFFSET.x, &LEDGE_NORMAL_ANG.pan);
				vec_add(&LEDGE_OFFSET.x, &LEDGE_HIT_VEC.x);
				
				my->x -= (my->x - LEDGE_OFFSET.x) * 0.9 * time_step;
				my->y -= (my->y - LEDGE_OFFSET.y) * 0.9 * time_step;
				my->z -= (my->z - (LEDGE_TARGET_Z - 32)) * 0.9 * time_step;
				
				if(abs(ang(ang(LEDGE_NORMAL_ANG.pan - 180) - camera->pan)) <= 2.5 && abs(ang(25 - camera->tilt)) <= 2.5 && vec_dist(&my->x, vector(LEDGE_OFFSET.x, LEDGE_OFFSET.y, LEDGE_TARGET_Z - 32)) <= 2.5){
					// hang on the ledge:
					my->MOVE_TYPE = MOVE_HUNG_ON_LEDGE;
				}
			}
			
			// move on the ledge, switch to climb state or jump off:
			if(my->MOVE_TYPE == MOVE_HUNG_ON_LEDGE){
				VECTOR TEMP_VEC;
				vec_set(&TEMP_VEC.x, vector(64, 0, 16));
				vec_rotate(&TEMP_VEC.x, vector(my->pan, 0, 0));
				vec_add(&TEMP_VEC.x, &my->x);
				c_trace(vector(my->x, my->y, my->z + 16), &TEMP_VEC.x, TRACE_FLAGS | IGNORE_YOU | IGNORE_MODELS | SCAN_TEXTURE | USE_BOX);
				if(HIT_TARGET){
					if(tex_flag2){ vec_set(&LEDGE_HIT_VEC, &target.x); vec_to_angle(&LEDGE_NORMAL_ANG.pan, &normal.x); }
				}				
				my->pan -= ang(my->pan - ang(LEDGE_NORMAL_ANG.pan - 180)) * 0.5 * time_step;
				
				LEDGE_CAM_INPUT = clamp(LEDGE_CAM_INPUT - mickey.x / 6.5 * MOUSE_SPEED_FACTOR, -170, 170);
				camera->pan = LEDGE_CAM_INPUT + my->pan;
				camera->tilt = clamp(camera->tilt - mickey.y / 6.5 * MOUSE_SPEED_FACTOR, -15, 85);
				
				// fix little gap between player and surface:
				c_move(my, nullvector, vec_rotate(vector(time_step, 0, 0), vector(my->pan, 0, 0)), MOVE_FLAGS | IGNORE_YOU | IGNORE_MODELS);
				// fix our Z position:
				my->z = (LEDGE_TARGET_Z - 32);
				
				// if we can walk on this ledge:
				if(LEDGE_ALLOW_WALK == 1){
					// input from player:
					LEDGE_INPUT = LEDGE_SPEED * (key_a - key_d);
					
					// no smooth but rhytmical movement:
					LEDGE_MOVE_WAVE = LEDGE_MOVE_AMPL * sin(total_ticks * LEDGE_MOVE_RATE);
					LEDGE_INPUT *= 0.5 + (0.25 * LEDGE_MOVE_WAVE);
					
					var LEDGE_FRIC = maxv((1 - time_step * LEDGE_FRICTION), 0);
					LEDGE_VEL = (time_step * LEDGE_INPUT) + (LEDGE_FRIC * LEDGE_VEL);
					LEDGE_DIST = LEDGE_VEL * time_step;
					LEDGE_DIST = LEDGE_DIST * GLOBAL_MOVEMENT_SCALE;
					if(LEDGE_LEFT_OUT == 0){ LEDGE_INPUT = minv(LEDGE_INPUT, 0); LEDGE_VEL = LEDGE_DIST = LEDGE_INPUT; }
					if(LEDGE_RIGHT_OUT == 0){ LEDGE_INPUT = maxv(LEDGE_INPUT, 0); LEDGE_VEL = LEDGE_DIST = LEDGE_INPUT; }
					MOVE_DIST = MOVING_SPEED = IS_MOVING = 0;
					if(abs(LEDGE_CAM_INPUT) < LEDGE_JUMP_ANG){
						move_friction = 0;
						c_ignore(PUSH_GROUP, PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);
						MOVE_DIST = c_move(my, vector(0, LEDGE_DIST, 0), nullvector, MOVE_FLAGS | IGNORE_YOU | IGNORE_MODELS | GLIDE);
						if(MOVE_DIST > 0){ MOVING_SPEED = MOVE_DIST / time_step; IS_MOVING = 1; }
						else{ MOVING_SPEED = 0; IS_MOVING = 0; }
					}
				}
				
				VECTOR LEFT_VEC, RIGHT_VEC;
				vec_set(&LEFT_VEC.x, vector(16, 16, 24));
				vec_rotate(&LEFT_VEC.x, vector(my->pan, 0, 0));
				vec_add(&LEFT_VEC.x, &my->x);
				vec_set(&RIGHT_VEC.x, vector(16, -16, 24));
				vec_rotate(&RIGHT_VEC.x, vector(my->pan, 0, 0));
				vec_add(&RIGHT_VEC.x, &my->x);
				LEDGE_LEFT_OUT = region_check(reg_ledge_str, &LEFT_VEC.x, &LEFT_VEC.x);
				LEDGE_RIGHT_OUT = region_check(reg_ledge_str, &RIGHT_VEC.x, &RIGHT_VEC.x);
				
				if(key_space){
					if(JUMP_KEY_OFF == 1){
						
						VECTOR LEFT_TRACE, RIGHT_TRACE;
						vec_set(&LEFT_TRACE.x, vector(44, 32, 24));
						vec_rotate(&LEFT_TRACE.x, vector(my->pan, 0, 0));
						vec_add(&LEFT_TRACE.x, &my->x);
						vec_set(&RIGHT_TRACE.x, vector(44, -32, 24));
						vec_rotate(&RIGHT_TRACE.x, vector(my->pan, 0, 0));
						vec_add(&RIGHT_TRACE.x, &my->x);
						LEDGE_LEFT_OUT = c_trace(&my->x, &LEFT_TRACE.x, TRACE_FLAGS | IGNORE_MODELS);
						LEDGE_RIGHT_OUT = c_trace(&my->x, &RIGHT_TRACE.x, TRACE_FLAGS | IGNORE_MODELS);
						
						if(LEDGE_LEFT_OUT != 0 && LEDGE_RIGHT_OUT != 0){
							// if we are looking for a ledge around:
							if(abs(LEDGE_CAM_INPUT) > LEDGE_JUMP_ANG){ LEDGE_ALLOW_DETECT = 1; LEDGE_ALLOW_WALK = 1; HERO_ALLOW_ROTATE = 1; JUMP_FROM_SOMETHING = 1; my->MOVE_TYPE = MOVE_ON_FOOT; }
							else{
								VECTOR MAX_VEC; vec_set(&MAX_VEC, &my->x); vec_add(&MAX_VEC, &my->max_x);
								// we need to climb up and jump over the handrail?
								if(region_check(reg_jump_over_str, &MAX_VEC.x, vector(MAX_VEC.x, MAX_VEC.y, MAX_VEC.z + 32))){
									
									vec_set(vector(my->x, my->y, 0), vector(LEDGE_OFFSET.x, LEDGE_OFFSET.y, 0));
									vec_set(&LEDGE_OFFSET.x, vector(0, 0, HANDRAIL_Z_HEIGHT));
									vec_rotate(&LEDGE_OFFSET.x, &my->pan);
									vec_add(&LEDGE_OFFSET.x, &my->x);
									my->MOVE_TYPE = MOVE_GET_JUMP_OVER;
									
								}
								else{								
									// check if we can climb up this ledge (make a trace)
									VECTOR CLIMB_VEC;
									vec_set(&CLIMB_VEC.x, vector(16, 0, 63));
									vec_rotate(&CLIMB_VEC.x, vector(my->pan, 0, 0));
									vec_add(&CLIMB_VEC.x, &my->x);
									var HIT = c_trace(vector(my->x, my->y, my->z + 63), &CLIMB_VEC.x, TRACE_FLAGS | IGNORE_YOU | IGNORE_MODELS | USE_BOX);
									if(HIT == 0){// we can climb?
										vec_set(&LEDGE_OFFSET.x, vector(36, 0, 64));
										vec_rotate(&LEDGE_OFFSET.x, &my->pan);
										vec_add(&LEDGE_OFFSET.x, &my->x);
										my->MOVE_TYPE = MOVE_CLIMB_LEDGE;
									}
								}
							}
						}
						JUMP_KEY_OFF = 0;
					}
				}
				else{ JUMP_KEY_OFF = 1; }
				if(key_s){ LEDGE_ALLOW_DETECT = 1; LEDGE_ALLOW_WALK = 1; HERO_ALLOW_ROTATE = 1; my->MOVE_TYPE = MOVE_ON_FOOT; }
			}
			
			// climb the ledge up state:
			if(my->MOVE_TYPE == MOVE_CLIMB_LEDGE){
				camera->pan -= ang(camera->pan - ang(LEDGE_NORMAL_ANG.pan - 180)) * 0.5 * time_step;
				camera->tilt -= ang(camera->tilt - 0) * 0.5 * time_step;
				my->pan = camera->pan;
				my->x -= (my->x - LEDGE_OFFSET.x) * 0.2 * time_step;
				my->y -= (my->y - LEDGE_OFFSET.y) * 0.2 * time_step;
				my->z -= (my->z - LEDGE_OFFSET.z) * 0.4 * time_step;
				if(vec_dist(&my->x, &LEDGE_OFFSET.x) <= 5){ LEDGE_ALLOW_DETECT = 1; LEDGE_ALLOW_WALK = 1; HERO_ALLOW_ROTATE = 1; my->MOVE_TYPE = MOVE_ON_FOOT; }
			}
			
			// moving on walkable pipe:
			if(my->MOVE_TYPE == MOVE_ON_PIPE){
				// reset all previous movement on foot:
				vec_fill(&FORCE.x, 0); vec_fill(&ABS_FORCE.x, 0);
				vec_fill(&DIST.x, 0); vec_fill(&ABS_DIST.x, 0);
				vec_fill(&my->SPEED_X, 0);
				JUMP_TARGET = 0;
				MOVE_DIST = MOVING_SPEED = IS_MOVING = 0;
				// reset pushing from wall force:
				vec_fill(&FOOT_ONE_VEC.x, 0); vec_fill(&FOOT_TWO_VEC.x, 0);
				vec_fill(&FOOT_DIFF_VEC.x, 0);
				FOOT_HIT_X = FOOT_HIT_Y = FOOT_HIT_MIDDLE = 0;			
				// reset all previous movement on ledge:
				LEDGE_INPUT = LEDGE_VEL = LEDGE_DIST = 0;
				HERO_ALLOW_TIGHTROPE_FALL = 0;
				HERO_TIGHTROPE_SIDE -= (HERO_TIGHTROPE_SIDE - 0) * time_step;
				vec_fill(&FORCE_ROPE.x, 0); vec_fill(&DIST_ROPE.x, 0);
				vec_fill(&FORCE_TIGHTROPE.x, 0); vec_fill(&DIST_TIGHTROPE.x, 0);
				
				var DIR_ON_PIPE = 0;
				if(camera->tilt > HERO_CLIMB_TILT_LIMIT){ DIR_ON_PIPE = maxv(minv(abs((camera->tilt + 70) / 100), 1), 0.3); }
				else{ DIR_ON_PIPE = clamp(((camera->tilt + 10) / 100) - 0.5, -1, -0.3); }
				FORCE_PIPE.z = PIPE_SPEED * (key_w - key_s) * DIR_ON_PIPE;					
				if(key_shift){ FORCE_PIPE.z *= 1.25; }
				
				// no smooth but rhytmical movement:
				PIPE_MOVE_WAVE = PIPE_MOVE_AMPL * sin(total_ticks * PIPE_MOVE_RATE);
				FORCE_PIPE.z *= 0.5 + (0.25 * PIPE_MOVE_WAVE);
				
				accelerate(&DIST_PIPE.z, FORCE_PIPE.z * time_step, 1);
				DIST_PIPE.x = DIST_PIPE.y = 0;					
				
				if(abs(LEDGE_CAM_INPUT) < (LEDGE_JUMP_ANG - 5)){
					move_friction = 0;
					c_ignore(PUSH_GROUP, PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);
					MOVE_DIST = c_move(my, nullvector, vector(0, 0, DIST_PIPE.z), MOVE_FLAGS | GLIDE);
					if(MOVE_DIST > 0){ MOVING_SPEED = MOVE_DIST / time_step; IS_MOVING = 1; }
					else{ MOVING_SPEED = 0; IS_MOVING = 0; }
				}
				
				// limit player's movement on Z axis:
				my->z = clamp(my->z, PIPE_BOTTOM_VEC.z + 32, PIPE_TOP_VEC.z - 32);
				
				LEDGE_CAM_INPUT = clamp(LEDGE_CAM_INPUT - mickey.x / 6.5 * MOUSE_SPEED_FACTOR, -170, 170);
				camera->pan = LEDGE_CAM_INPUT + my->pan;
				camera->tilt = clamp(camera->tilt - mickey.y / 6.5 * MOUSE_SPEED_FACTOR, -65, 65);
				
				if(key_space){
					if(JUMP_KEY_OFF == 1){
						if(abs(LEDGE_CAM_INPUT) > (LEDGE_JUMP_ANG - 15)){
							LEDGE_ALLOW_DETECT = 1;
							LEDGE_ALLOW_WALK = 1;
							HERO_ALLOW_ROTATE = 1;
							JUMP_FROM_SOMETHING = 1;
							FOOT_DISABLE = 1;
							my->MOVE_TYPE = MOVE_ON_FOOT;
						}
						JUMP_KEY_OFF = 0;
					}
				}
				else{ JUMP_KEY_OFF = 1; }
			}
			
			// moving on the rope:
			if(my->MOVE_TYPE == MOVE_ON_ROPE){
				// reset all previous movement on foot:
				vec_fill(&FORCE.x, 0); vec_fill(&ABS_FORCE.x, 0);
				vec_fill(&DIST.x, 0); vec_fill(&ABS_DIST.x, 0);
				vec_fill(&my->SPEED_X, 0);
				JUMP_TARGET = 0;
				MOVE_DIST = MOVING_SPEED = IS_MOVING = 0;
				// reset pushing from wall force:
				vec_fill(&FOOT_ONE_VEC.x, 0); vec_fill(&FOOT_TWO_VEC.x, 0);
				vec_fill(&FOOT_DIFF_VEC.x, 0);
				FOOT_HIT_X = FOOT_HIT_Y = FOOT_HIT_MIDDLE = 0;			
				// reset all previous movement on ledge:
				LEDGE_INPUT = LEDGE_VEL = LEDGE_DIST = 0;
				HERO_ALLOW_TIGHTROPE_FALL = 0;
				HERO_TIGHTROPE_SIDE -= (HERO_TIGHTROPE_SIDE - 0) * time_step;
				vec_fill(&FORCE_PIPE.x, 0); vec_fill(&FORCE_PIPE.x, 0);
				vec_fill(&FORCE_TIGHTROPE.x, 0); vec_fill(&DIST_TIGHTROPE.x, 0);
				
				var DIR_ON_ROPE = 0;
				if(camera->tilt > HERO_CLIMB_TILT_LIMIT){ DIR_ON_ROPE = maxv(minv(abs((camera->tilt + 70) / 100), 1), 0.3); }
				else{ DIR_ON_ROPE = clamp(((camera->tilt + 10) / 100) - 0.5, -1, -0.3); }
				FORCE_ROPE.z = ROPE_SPEED * (key_w - key_s) * DIR_ON_ROPE;					
				if(key_shift){ FORCE_ROPE.z *= 1.25; }
				
				// no smooth but rhytmical movement:
				ROPE_MOVE_WAVE = ROPE_MOVE_AMPL * sin(total_ticks * ROPE_MOVE_RATE);
				FORCE_ROPE.z *= 0.5 + (0.25 * ROPE_MOVE_WAVE);
				
				accelerate(&DIST_ROPE.z, FORCE_ROPE.z * time_step, 1);
				DIST_ROPE.x = DIST_ROPE.y = 0;					
				c_ignore(PUSH_GROUP, PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);
				move_friction = 0;
				MOVE_DIST = c_move(my, nullvector, vector(0, 0, DIST_ROPE.z), MOVE_FLAGS | GLIDE);
				if(MOVE_DIST > 0){ MOVING_SPEED = MOVE_DIST / time_step; IS_MOVING = 1; }
				else{ MOVING_SPEED = 0; IS_MOVING = 0; }
			}
			
			// moving on the tightrope:
			if(my->MOVE_TYPE == MOVE_ON_TIGHTROPE){
				// reset all previous movement on foot:
				vec_fill(&FORCE.x, 0); vec_fill(&ABS_FORCE.x, 0);
				vec_fill(&DIST.x, 0); vec_fill(&ABS_DIST.x, 0);
				vec_fill(&my->SPEED_X, 0);
				JUMP_TARGET = 0;
				MOVE_DIST = MOVING_SPEED = IS_MOVING = 0;
				// reset pushing from wall force:
				vec_fill(&FOOT_ONE_VEC.x, 0); vec_fill(&FOOT_TWO_VEC.x, 0);
				vec_fill(&FOOT_DIFF_VEC.x, 0);
				FOOT_HIT_X = FOOT_HIT_Y = FOOT_HIT_MIDDLE = 0;			
				// reset all previous movement on ledge:
				LEDGE_INPUT = LEDGE_VEL = LEDGE_DIST = 0;
				vec_fill(&FORCE_PIPE.x, 0); vec_fill(&FORCE_PIPE.x, 0);
				vec_fill(&FORCE_ROPE.x, 0); vec_fill(&DIST_ROPE.x, 0);
				
				FORCE_TIGHTROPE.x = TIGHTROPE_SPEED * (key_w - key_s);
				accelerate(&DIST_TIGHTROPE.x, FORCE_TIGHTROPE.x * time_step, 1);
				
				if(HERO_TIGHTROPE_SIDE == 0 && HERO_ALLOW_TIGHTROPE_FALL == 1){ HERO_TIGHTROPE_SIDE = 0.1 - random(0.2); }
				if(HERO_ALLOW_TIGHTROPE_FALL == 0){ HERO_TIGHTROPE_SIDE -= (HERO_TIGHTROPE_SIDE - 0) * time_step; }
				
				HERO_TIGHTROPE_SIDE += ((sign(HERO_TIGHTROPE_SIDE) * 0.25) - (key_a - key_d) * 0.5) * time_step;
				HERO_TIGHTROPE_SIDE = clamp(HERO_TIGHTROPE_SIDE, -6, 6);
				
				c_ignore(PUSH_GROUP, PLAYER_GROUP, SWITCH_ITEM_GROUP, 0);
				move_friction = 0;
				MOVE_DIST = c_move(my, vector(DIST_TIGHTROPE.x, DIST_TIGHTROPE.y, 0), nullvector, MOVE_FLAGS | GLIDE);
				if(MOVE_DIST > 0){ MOVING_SPEED = MOVE_DIST / time_step; IS_MOVING = 1; }
				else{ MOVING_SPEED = 0; IS_MOVING = 0; }
				
				LEDGE_CAM_INPUT = clamp(LEDGE_CAM_INPUT - mickey.x / 6.5 * MOUSE_SPEED_FACTOR, -15, 15);
				camera->pan = LEDGE_CAM_INPUT + my->pan;
				camera->tilt = clamp(camera->tilt - mickey.y / 6.5 * MOUSE_SPEED_FACTOR, -45, 15);
			}
			
			// lerp to the climbable handrail position:
			if(my->MOVE_TYPE == MOVE_GET_JUMP_OVER){
				// reset all previous movement on foot:
				vec_fill(&FORCE.x, 0); vec_fill(&ABS_FORCE.x, 0);
				vec_fill(&DIST.x, 0); vec_fill(&ABS_DIST.x, 0);
				vec_fill(&my->SPEED_X, 0);
				JUMP_TARGET = 0;
				MOVE_DIST = MOVING_SPEED = IS_MOVING = 0;
				// reset pushing from wall force:
				vec_fill(&FOOT_ONE_VEC.x, 0); vec_fill(&FOOT_TWO_VEC.x, 0);
				vec_fill(&FOOT_DIFF_VEC.x, 0);
				FOOT_HIT_X = FOOT_HIT_Y = FOOT_HIT_MIDDLE = 0;			
				// reset all previous movement on ledge:
				LEDGE_INPUT = LEDGE_VEL = LEDGE_DIST = 0;
				HERO_ALLOW_TIGHTROPE_FALL = 0;
				HERO_TIGHTROPE_SIDE -= (HERO_TIGHTROPE_SIDE - 0) * time_step;
				vec_fill(&FORCE_PIPE.x, 0); vec_fill(&FORCE_PIPE.x, 0);
				vec_fill(&FORCE_ROPE.x, 0); vec_fill(&DIST_ROPE.x, 0);
				vec_fill(&FORCE_TIGHTROPE.x, 0); vec_fill(&DIST_TIGHTROPE.x, 0);
				
				LEDGE_CAM_INPUT = clamp(LEDGE_CAM_INPUT - mickey.x / 6.5 * MOUSE_SPEED_FACTOR, -170, 170);
				camera->pan = LEDGE_CAM_INPUT + my->pan;
				camera->tilt = clamp(camera->tilt - mickey.y / 6.5 * MOUSE_SPEED_FACTOR, -15, 85);
				
				my->z -= (my->z - LEDGE_OFFSET.z) * 0.4 * time_step;
				if(key_space && vec_dist(vector(0, 0, my->z), vector(0, 0, LEDGE_OFFSET.z)) <= 1){
					if(JUMP_KEY_OFF == 1){
						// if we are looking for a ledge around:
						if(abs(LEDGE_CAM_INPUT) > (LEDGE_JUMP_ANG + 5)){ LEDGE_ALLOW_DETECT = 1; LEDGE_ALLOW_WALK = 1; HERO_ALLOW_ROTATE = 1; JUMP_FROM_SOMETHING = 1; my->MOVE_TYPE = MOVE_ON_FOOT; }
						else{
							LEDGE_OFFSET.z += 40;
							my->MOVE_TYPE = MOVE_JUMP_OVER_UP;
						}
						JUMP_KEY_OFF = 0;
					}
				}
				else{ JUMP_KEY_OFF = 1; }
				if(key_s){ my->MOVE_TYPE = MOVE_GRAB_LEDGE; }
			}
			
			// switch to climb up handrail step:
			if(my->MOVE_TYPE == MOVE_JUMP_OVER_UP){
				LEDGE_CAM_INPUT = clamp(LEDGE_CAM_INPUT - mickey.x / 6.5 * MOUSE_SPEED_FACTOR, -170, 170);
				camera->pan = LEDGE_CAM_INPUT + my->pan;
				camera->tilt = clamp(camera->tilt - mickey.y / 6.5 * MOUSE_SPEED_FACTOR, -15, 85);
				my->z -= (my->z - LEDGE_OFFSET.z) * 0.4 * time_step;
				if(vec_dist(vector(0, 0, my->z), vector(0, 0, LEDGE_OFFSET.z)) <= 15){
					vec_set(&LEDGE_OFFSET.x, vector(48, 0, 0));
					vec_rotate(&LEDGE_OFFSET.x, &my->pan);
					vec_add(&LEDGE_OFFSET.x, &my->x);
					my->MOVE_TYPE = MOVE_JUMP_OVER_FORWARD;
				}
			}
			
			// switch to jump over the handrail:
			if(my->MOVE_TYPE == MOVE_JUMP_OVER_FORWARD){
				LEDGE_CAM_INPUT = clamp(LEDGE_CAM_INPUT - mickey.x / 6.5 * MOUSE_SPEED_FACTOR, -95, 95);
				camera->pan = LEDGE_CAM_INPUT + my->pan;
				camera->tilt = clamp(camera->tilt - mickey.y / 6.5 * MOUSE_SPEED_FACTOR, -90, 90);
				my->x -= (my->x - LEDGE_OFFSET.x) * 0.3 * time_step;
				my->y -= (my->y - LEDGE_OFFSET.y) * 0.3 * time_step;
				if(vec_dist(vector(my->x, my->y, 0), vector(LEDGE_OFFSET.x, LEDGE_OFFSET.y, 0)) <= 2.5){ LEDGE_ALLOW_DETECT = 1; HERO_ALLOW_ROTATE = 1; my->MOVE_TYPE = MOVE_ON_FOOT; }
			}
			
			// switch to grab step state:
			if(my->MOVE_TYPE == MOVE_GRAB_STEP){
				// reset all previous movement on foot:
				vec_fill(&FORCE.x, 0); vec_fill(&ABS_FORCE.x, 0);
				vec_fill(&DIST.x, 0); vec_fill(&ABS_DIST.x, 0);
				vec_fill(&my->SPEED_X, 0);
				JUMP_TARGET = 0;
				MOVE_DIST = MOVING_SPEED = IS_MOVING = 0;
				// reset pushing from wall force:
				vec_fill(&FOOT_ONE_VEC.x, 0); vec_fill(&FOOT_TWO_VEC.x, 0);
				vec_fill(&FOOT_DIFF_VEC.x, 0);
				FOOT_HIT_X = FOOT_HIT_Y = FOOT_HIT_MIDDLE = 0;			
				// reset all previous movement on ledge:
				LEDGE_INPUT = LEDGE_VEL = LEDGE_DIST = 0;
				HERO_ALLOW_TIGHTROPE_FALL = 0;
				HERO_TIGHTROPE_SIDE -= (HERO_TIGHTROPE_SIDE - 0) * time_step;
				vec_fill(&FORCE_PIPE.x, 0); vec_fill(&FORCE_PIPE.x, 0);
				vec_fill(&FORCE_ROPE.x, 0); vec_fill(&DIST_ROPE.x, 0);
				vec_fill(&FORCE_TIGHTROPE.x, 0); vec_fill(&DIST_TIGHTROPE.x, 0);
				
				vec_set(&LEDGE_OFFSET.x, vector(16, 0, 0));
				vec_rotate(&LEDGE_OFFSET.x, &LEDGE_NORMAL_ANG.pan);
				vec_add(&LEDGE_OFFSET.x, &LEDGE_HIT_VEC.x);
				
				camera->pan -= ang(camera->pan - ang(LEDGE_NORMAL_ANG.pan - 180)) * 0.25 * time_step;
				camera->tilt -= ang(camera->tilt - 5) * 0.5 * time_step;
				my->pan = camera->pan;
				
				my->x -= (my->x - LEDGE_OFFSET.x) * 0.2 * time_step;
				my->y -= (my->y - LEDGE_OFFSET.y) * 0.2 * time_step;
				
				if(abs(ang(ang(LEDGE_NORMAL_ANG.pan - 180) - my->pan)) <= 2.5 && abs(ang(5 - camera->tilt)) <= 2.5 && vec_dist(vector(my->x, my->y, 0), vector(LEDGE_OFFSET.x, LEDGE_OFFSET.y, 0)) <= 2.5){
					vec_set(vector(my->x, my->y, 0), vector(LEDGE_OFFSET.x, LEDGE_OFFSET.y, 0));
					vec_set(&LEDGE_OFFSET.x, vector(48, 0, STEP_Z_HEIGHT + 32));
					vec_rotate(&LEDGE_OFFSET.x, &my->pan);
					vec_add(&LEDGE_OFFSET.x, &my->x);
					my->MOVE_TYPE = MOVE_CLIMB_STEP;
				}
			}
			
			// switch to climb up state:
			if(my->MOVE_TYPE == MOVE_CLIMB_STEP){
				camera->pan -= ang(camera->pan - ang(LEDGE_NORMAL_ANG.pan - 180)) * 0.5 * time_step;
				camera->tilt -= ang(camera->tilt - 0) * 0.5 * time_step;
				my->pan = camera->pan;
				my->x -= (my->x - LEDGE_OFFSET.x) * 0.2 * time_step;
				my->y -= (my->y - LEDGE_OFFSET.y) * 0.2 * time_step;
				my->z -= (my->z - LEDGE_OFFSET.z) * 0.4 * time_step;
				if(vec_dist(&my->x, &LEDGE_OFFSET.x) <= 5){ LEDGE_ALLOW_DETECT = 1; LEDGE_ALLOW_WALK = 1; HERO_ALLOW_ROTATE = 1; my->MOVE_TYPE = MOVE_ON_FOOT; }
			}
			
		}
		else{
			// reset all previous movement on foot:
			vec_fill(&FORCE.x, 0); vec_fill(&ABS_FORCE.x, 0);
			vec_fill(&DIST.x, 0); vec_fill(&ABS_DIST.x, 0);
			vec_fill(&my->SPEED_X, 0);
			JUMP_TARGET = 0;
			MOVE_DIST = MOVING_SPEED = IS_MOVING = 0;
			// reset pushing from wall force:
			vec_fill(&FOOT_ONE_VEC.x, 0); vec_fill(&FOOT_TWO_VEC.x, 0);
			vec_fill(&FOOT_DIFF_VEC.x, 0);
			FOOT_HIT_X = FOOT_HIT_Y = FOOT_HIT_MIDDLE = 0;			
			// reset all previous movement on ledge:
			LEDGE_INPUT = LEDGE_VEL = LEDGE_DIST = LEDGE_CAM_INPUT = 0;
			HERO_ALLOW_TIGHTROPE_FALL = 0;
			HERO_TIGHTROPE_SIDE -= (HERO_TIGHTROPE_SIDE - 0) * time_step;
			vec_fill(&FORCE_PIPE.x, 0); vec_fill(&FORCE_PIPE.x, 0);
			vec_fill(&FORCE_ROPE.x, 0); vec_fill(&DIST_ROPE.x, 0);
			vec_fill(&FORCE_TIGHTROPE.x, 0); vec_fill(&DIST_TIGHTROPE.x, 0);
		}
		
		if(HERO_ALLOW_ROTATE == 1){
			camera->pan = cycle(camera->pan - mickey.x / 6.5 * MOUSE_SPEED_FACTOR, 0, 360);
			camera->tilt = clamp(camera->tilt - mickey.y / 6.5 * MOUSE_SPEED_FACTOR, -90, 90);
			my->pan = camera->pan;
		}
		camera->roll = HERO_TIGHTROPE_SIDE * 4;
		
		/*
		my->skill1 -= 0.3 * mickey.z;
		my->skill1 = clamp(my->skill1, 64, 256);		
		CAM_POS.x = my->x + fcos((camera->pan), fcos(camera->tilt, -my->skill1));
		CAM_POS.y = my->y + fsin((camera->pan), fcos(camera->tilt, -my->skill1));
		CAM_POS.z = my->z + 16 + fsin(camera->tilt, -my->skill1);
		*/
		
		vec_set(&CAM_POS.x, vector(0, -HERO_TIGHTROPE_SIDE * 2, 24));
		vec_rotate(&CAM_POS.x, vector(camera->pan, 0, 0));
		vec_add(&CAM_POS.x, &my->x);
		
		vec_set(&camera->x, &CAM_POS.x);
		
		CAM_DIR.x = cosv(camera->pan);
		CAM_DIR.y = sinv(camera->pan);
		CAM_DIR.z = 0;
		vec_normalize(&CAM_DIR.x, 1);
		
		wait(1);
	}
}

// walkable pipe event:
void pipe_event(){
	if(event_type == EVENT_PUSH){
		if(you == HERO_ENT){
			
			VECTOR PIPE_DIFF;
			vec_fill(&PIPE_DIFF.x, 0);
			
			vec_diff(&PIPE_DIFF.x, &my->x, &HERO_ENT->x);
			PIPE_DIFF.z = 0;
			vec_normalize(&PIPE_DIFF.x, 1);
			var LOOK_AT_PIPE = vec_dot(&PIPE_DIFF.x, &CAM_DIR.x);
			if(LOOK_AT_PIPE < 0.85){ return; }
			
			my->emask &= ~ENABLE_PUSH;
			
			HERO_ALLOW_MOVE = HERO_ALLOW_ROTATE = 0;			
			
			VECTOR LINE_VEC, OFFSET_VEC;
			vec_set(&PIPE_TOP_VEC.x, vector(0, 0, my->max_z));
			vec_rotate(&PIPE_TOP_VEC.x, &my->pan);
			vec_add(&PIPE_TOP_VEC.x, &my->x);
			vec_set(&PIPE_BOTTOM_VEC.x, vector(0, 0, my->min_z));
			vec_rotate(&PIPE_BOTTOM_VEC.x, &my->pan);
			vec_add(&PIPE_BOTTOM_VEC.x, &my->x);
			
			while(my){
				
				VECTOR AP, AB;
				vec_diff(&AP.x, &HERO_ENT->x, &PIPE_TOP_VEC.x);
				vec_diff(&AB.x, &PIPE_BOTTOM_VEC.x, &PIPE_TOP_VEC.x);				
				vec_set(&LINE_VEC.x, &PIPE_TOP_VEC.x);
				vec_add(&LINE_VEC.x, vec_scale(&AB.x, vec_dot(&AP.x, &AB.x) / vec_dot(&AB.x, &AB.x)));
				
				vec_set(&OFFSET_VEC.x, vector(16, 0, 0));
				vec_rotate(&OFFSET_VEC.x, vector(my->pan, 0, 0));
				vec_add(&OFFSET_VEC.x, &LINE_VEC.x);
				OFFSET_VEC.z = clamp(OFFSET_VEC.z, PIPE_BOTTOM_VEC.z + 32, PIPE_TOP_VEC.z - 32);
				
				HERO_ENT->x -= (HERO_ENT->x - OFFSET_VEC.x) * 0.9 * time_step;
				HERO_ENT->y -= (HERO_ENT->y - OFFSET_VEC.y) * 0.9 * time_step;
				HERO_ENT->z -= (HERO_ENT->z - OFFSET_VEC.z) * 0.9 * time_step;
				
				camera->pan -= ang(camera->pan - ang(my->pan - 180)) * 0.35 * time_step;
				camera->tilt -= ang(camera->tilt - 0) * 0.35 * time_step;				
				HERO_ENT->pan = camera->pan;				
				if(abs(ang(ang(my->pan - 180) - camera->pan)) <= 0.15 && vec_dist(&HERO_ENT->x, &OFFSET_VEC.x) <= 5){ break; }				
				
				wait(1);
			}
			
			vec_set(&HERO_ENT->x, &OFFSET_VEC.x);
			camera->pan = ang(my->pan - 180);
			camera->tilt = 0;
			
			HERO_ENT->MOVE_TYPE = MOVE_ON_PIPE;
			HERO_ALLOW_MOVE = 1;
			
			while(HERO_ENT->MOVE_TYPE == MOVE_ON_PIPE){ wait(1); }			
			while(vec_dist(vector(HERO_ENT->x, HERO_ENT->y, 0), vector(OFFSET_VEC.x, OFFSET_VEC.y, 0)) < 32){ wait(1); }
			my->emask |= ENABLE_PUSH;
		}
	}
}

// walkable pipe:
action actPipe(){
	c_setminmax(my);
	set(my, POLYGON | TRANSLUCENT);
	reset(my, DYNAMIC);
	
	my->group = PUSH_GROUP;
	my->push = PUSH_GROUP;

	my->emask |= (ENABLE_PUSH);
	my->event = pipe_event;
	
	int i = 0;
	var rot = 0;
	
	for(i = 0; i < 4; i++){
		VECTOR TEMP_VEC;
		vec_set(&TEMP_VEC.x, vector(16, 0, 0));
		vec_rotate(&TEMP_VEC.x, vector(rot + 90 * i, 0, 0));
		vec_add(&TEMP_VEC.x, &my->x);
		
		c_trace(&my->x, &TEMP_VEC.x, TRACE_FLAGS | IGNORE_MODELS | USE_BOX);
		if(HIT_TARGET){
			vec_to_angle(&my->pan, &normal);
			my->tilt = my->roll = 0;
			break;
		}
	}
}

// rope event:
void rope_event(){
	if(event_type == EVENT_SHOOT){
		if(you == HERO_ENT){
			
			VECTOR ROPE_DIFF;
			vec_fill(&ROPE_DIFF.x, 0);
			
			vec_diff(&ROPE_DIFF.x, &my->x, &HERO_ENT->x);
			ROPE_DIFF.z = 0;
			vec_normalize(&ROPE_DIFF.x, 1);
			var LOOK_AT_ROPE = vec_dot(&ROPE_DIFF.x, &CAM_DIR.x);
			if(LOOK_AT_ROPE < 0.85){ return; }
			
			my->emask &= ~ENABLE_SHOOT;
			
			HERO_ENT->MOVE_TYPE = MOVE_ON_ROPE;
			HERO_ALLOW_MOVE = HERO_ALLOW_ROTATE = 0;			
			
			VECTOR TEMP_VEC, DIR_VEC;
			vec_fill(&TEMP_VEC.x, 0); vec_fill(&DIR_VEC.x, 0);
			
			ANGLE TEMP_ANG;
			vec_fill(&TEMP_ANG.pan, 0);
			
			var ROPE_MOVE_ID = -1;
			
			while(HERO_ENT){
				
				vec_set(&DIR_VEC.x, &my->x); 
				vec_sub(&DIR_VEC.x, &HERO_ENT->x);
				vec_to_angle(&TEMP_ANG.pan, &DIR_VEC.x);
				
				camera->pan -= ang(camera->pan - TEMP_ANG.pan) * 0.35 * time_step;
				camera->tilt -= ang(camera->tilt - 0) * 0.35 * time_step;				
				if(abs(ang(TEMP_ANG.pan - camera->pan)) <= 0.25){ break; }
				
				vec_set(&TEMP_VEC.x, vector(15, 0, 0));
				vec_rotate(&TEMP_VEC.x, vector(HERO_ENT->pan, 0, 0));
				vec_inverse(&TEMP_VEC.x);
				vec_add(&TEMP_VEC.x, &my->x);
				
				HERO_ENT->x -= (HERO_ENT->x - TEMP_VEC.x) * 0.9 * time_step;
				HERO_ENT->y -= (HERO_ENT->y - TEMP_VEC.y) * 0.9 * time_step;
				
				wait(1);
			}
			
			HERO_ALLOW_MOVE = 1;
			
			while(HERO_ENT){
				camera->pan = cycle(camera->pan - mickey.x / 6.5 * MOUSE_SPEED_FACTOR, 0, 360);
				camera->tilt = clamp(camera->tilt - mickey.y / 6.5 * MOUSE_SPEED_FACTOR, -65, 65);
				HERO_ENT->pan = camera->pan;
				
				vec_set(&TEMP_VEC.x, vector(15, 0, 0));
				vec_rotate(&TEMP_VEC.x, vector(camera->pan, 0, 0));
				vec_inverse(&TEMP_VEC.x);
				vec_add(&TEMP_VEC.x, &my->x);
				
				HERO_ENT->x = TEMP_VEC.x;
				HERO_ENT->y = TEMP_VEC.y;
				
				if(HERO_ENT->z + HERO_ENT->min_z < my->min_z + my->z + 5 && key_s && camera->tilt > HERO_CLIMB_TILT_LIMIT){ ROPE_MOVE_ID = 1; break; }
				if(HERO_ENT->z + HERO_ENT->min_z < my->min_z + my->z + 5 && key_w && camera->tilt < HERO_CLIMB_TILT_LIMIT){ ROPE_MOVE_ID = 1; break; }
				if(key_space){ ROPE_MOVE_ID = -1; JUMP_FROM_SOMETHING = 1; set(my, PASSABLE); break; }
				
				wait(1);
			}
			
			while(HERO_ENT){
				if(ROPE_MOVE_ID == -1){ break; } // jumped away, then don't do lerping:
				// if we've riched the buttom:
				if(ROPE_MOVE_ID == 1){
					vec_set(&TEMP_VEC.x, vector(15, 0, 0));
					vec_rotate(&TEMP_VEC.x, vector(camera->pan, 0, 0));
					vec_inverse(&TEMP_VEC.x);
					vec_add(&TEMP_VEC.x, &my->x);
					TEMP_VEC.z = HERO_ENT->z;
					HERO_ENT->x -= (HERO_ENT->x - TEMP_VEC.x) * 0.5 * time_step;
					HERO_ENT->y -= (HERO_ENT->y - TEMP_VEC.y) * 0.5 * time_step;
				}
				if(vec_dist(vector(HERO_ENT->x, HERO_ENT->y, 0), vector(TEMP_VEC.x, TEMP_VEC.y, 0)) <= 5){ break; }
				wait(1);
			}
			HERO_ALLOW_ROTATE = 1;
			HERO_ENT->MOVE_TYPE = MOVE_ON_FOOT;
			while(vec_dist(vector(HERO_ENT->x, HERO_ENT->y, 0), vector(my->x, my->y, 0)) < 32){ wait(1); }
			reset(my, PASSABLE);
			my->emask |= ENABLE_SHOOT;
		}
	}
}

// rope action:
action actRope(){
	c_setminmax(my);
	set(my, POLYGON | FLAG2 | TRANSLUCENT);
	reset(my, DYNAMIC);
	
	my->group = OBSTACLE_GROUP;
	my->push = OBSTACLE_GROUP;

	my->emask |= ENABLE_SHOOT;
	my->event = rope_event;
}

// tightrope event:
void tightrope_event(){
	if(event_type == EVENT_PUSH){
		if(you == HERO_ENT){
			
			if(abs((my->z) - (HERO_ENT->z - 32)) > 8){ return; }
			if(vec_dist(vector(HERO_ENT->x, HERO_ENT->y, 0), vector(my->x, my->y, 0)) > my->max_x - 16){ return; }
			
			VECTOR TIGHT_START_VEC, TIGHT_END_VEC, LINE_VEC, OFFSET_VEC;
			vec_set(&TIGHT_START_VEC.x, vector(my->min_x, 0, 0));
			vec_rotate(&TIGHT_START_VEC.x, &my->pan);
			vec_add(&TIGHT_START_VEC.x, &my->x);
			vec_set(&TIGHT_END_VEC.x, vector(my->max_x, 0, 0));
			vec_rotate(&TIGHT_END_VEC.x, &my->pan);
			vec_add(&TIGHT_END_VEC.x, &my->x);
			
			VECTOR START_DIFF, END_DIFF;
			vec_diff(&START_DIFF.x, &TIGHT_START_VEC.x, &HERO_ENT->x);
			START_DIFF.z = 0;
			vec_normalize(&START_DIFF.x, 1);
			var LOOK_AT_START = vec_dot(&START_DIFF.x, &CAM_DIR.x);
			vec_diff(&END_DIFF.x, &TIGHT_END_VEC.x, &HERO_ENT->x);
			END_DIFF.z = 0;
			vec_normalize(&END_DIFF.x, 1);
			var LOOK_AT_END = vec_dot(&END_DIFF.x, &CAM_DIR.x);
			
			if(LOOK_AT_START < 0.8 && LOOK_AT_END < 0.8){ return; }
			if(vec_dist(vector(HERO_ENT->x, HERO_ENT->y, 0), vector(my->x, my->y, 0)) < my->max_x - 32){ return; }
			
			my->emask &= ~ENABLE_PUSH;
			
			ANGLE TARGET_ANG;
			HERO_ENT->MOVE_TYPE = MOVE_ON_TIGHTROPE;
			HERO_ALLOW_MOVE = HERO_ALLOW_ROTATE = 0;
			
			while(my){
				VECTOR AP, AB;
				vec_diff(&AP.x, &HERO_ENT->x, &TIGHT_START_VEC.x);
				vec_diff(&AB.x, &TIGHT_END_VEC.x, &TIGHT_START_VEC.x);
				vec_set(&LINE_VEC.x, &TIGHT_START_VEC.x);
				vec_add(&LINE_VEC.x, vec_scale(&AB.x, vec_dot(&AP.x, &AB.x) / vec_dot(&AB.x, &AB.x)));
				vec_set(&OFFSET_VEC.x, vector(LINE_VEC.x, LINE_VEC.y, LINE_VEC.z + 32 + my->min_z));
				HERO_ENT->x -= (HERO_ENT->x - OFFSET_VEC.x) * time_step;
				HERO_ENT->y -= (HERO_ENT->y - OFFSET_VEC.y) * time_step;
				HERO_ENT->z -= (HERO_ENT->z - OFFSET_VEC.z) * time_step;
				
				if(LOOK_AT_START > 0.8 && LOOK_AT_END > 0.8){ vec_to_angle(&TARGET_ANG.pan, vec_diff(NULL, &my->x, &OFFSET_VEC.x)); }
				if(LOOK_AT_START > 0.8 && LOOK_AT_END < 0 || vec_dist(&OFFSET_VEC.x, &TIGHT_START_VEC.x) > 128){ vec_to_angle(&TARGET_ANG.pan, vec_diff(NULL, &TIGHT_START_VEC.x, &OFFSET_VEC.x)); }
				if(LOOK_AT_START < 0 && LOOK_AT_END > 0.8 || vec_dist(&OFFSET_VEC.x, &TIGHT_END_VEC.x) > 128){ vec_to_angle(&TARGET_ANG.pan, vec_diff(NULL, &TIGHT_END_VEC.x, &OFFSET_VEC.x)); }
				camera->pan -= ang(camera->pan - TARGET_ANG.pan) * 0.5 * time_step;
				camera->tilt -= ang(camera->tilt - (-30)) * 0.5 * time_step;
				HERO_ENT->pan = camera->pan;
				if(abs(ang(TARGET_ANG.pan - camera->pan)) <= 0.5 && abs(ang((-30) - camera->tilt)) <= 0.5 && vec_dist(&HERO_ENT->x, &OFFSET_VEC.x) <= 0.5){ break; }
				wait(1);
			}
			
			vec_set(&HERO_ENT->x, &OFFSET_VEC.x);
			HERO_ENT->pan = camera->pan = TARGET_ANG.pan;
			camera->tilt = -30;
			HERO_ALLOW_MOVE = 1;
			
			while(my){
				VECTOR AP, AB;
				vec_diff(&AP.x, &HERO_ENT->x, &TIGHT_START_VEC.x);
				vec_diff(&AB.x, &TIGHT_END_VEC.x, &TIGHT_START_VEC.x);
				vec_set(&LINE_VEC.x, &TIGHT_START_VEC.x);
				vec_add(&LINE_VEC.x, vec_scale(&AB.x, vec_dot(&AP.x, &AB.x) / vec_dot(&AB.x, &AB.x)));
				vec_set(&OFFSET_VEC.x, vector(LINE_VEC.x, LINE_VEC.y, LINE_VEC.z + 32 + my->min_z));
				
				HERO_ALLOW_TIGHTROPE_FALL = 0;
				if(vec_dist(vector(HERO_ENT->x, HERO_ENT->y, 0), vector(my->x, my->y, 0)) > my->max_x + 16){ break; }
				if(vec_dist(vector(HERO_ENT->x, HERO_ENT->y, 0), vector(my->x, my->y, 0)) < my->max_x - 32){
					HERO_ALLOW_TIGHTROPE_FALL = 1;
					if(HERO_TIGHTROPE_SIDE > 5 || HERO_TIGHTROPE_SIDE < -5){ HERO_FALL_TIGHTROPE = 1; break; }
				}
				wait(1);
			}
			HERO_ALLOW_ROTATE = 1;
			HERO_ENT->MOVE_TYPE = MOVE_ON_FOOT;
			while(vec_dist(&HERO_ENT->x, &OFFSET_VEC.x) < 4){ wait(1); }
			my->emask |= ENABLE_PUSH;
		}
	}
}

// tightrope:
action actTightrope(){
	c_setminmax(my);
	set(my, POLYGON | INVISIBLE);
	reset(my, DYNAMIC);

	my->group = PUSH_GROUP;
	my->push = PUSH_GROUP;

	my->emask |= (ENABLE_PUSH);
	my->event = tightrope_event;
}

// fog, sky color setup:
void world_setup(){
	vec_set(&d3d_lodfactor, vector(12.5, 25, 50)); 
	sun_light = 50;
	camera->clip_near = 1;
	camera->clip_far = 2000;
	camera->fog_start = 200; 
	camera->fog_end = 1500; 
	fog_color = 4;
	vec_set(&d3d_fogcolor4, vector(128, 128, 128));
	vec_set(&sky_color, &d3d_fogcolor4);
	vec_set(&sun_color, &d3d_fogcolor4);
	vec_set(&ambient_color, &d3d_fogcolor4);
	set(camera, NOFLAG1);
}

// main game function:
void main(){
	video_mode = 8;
	video_window(NULL, NULL, NULL, "APARTMENT TEST!");	

	doppler_factor = 0;
	warn_level = 6;

	fps_max = 60;
	fps_min = 15;
	time_smooth = 0.9;

	freeze_mode = 1;
	level_load(level_test_str);
	wait(3);
	freeze_mode = 0;
	level_ent->group = LEVEL_GROUP;
	level_ent->push = LEVEL_GROUP;

	random_seed(0);	
	mouse_pointer = 0;

	world_setup();

	while(1){
		
		if(GAME_LOCK_MOUSE == 1){
			static var autolock_mouse_locked = 0;
			if(!autolock_mouse_locked && window_focus){
				autolock_mouse_locked = 1;
				RECT rect;
				GetClientRect(hWnd, &rect);
				ClientToScreen(hWnd, &rect);
				ClientToScreen(hWnd, &rect.right);
				ClipCursor(&rect);
			}
			if(autolock_mouse_locked && !window_focus){
				autolock_mouse_locked = 0;
				ClipCursor(NULL);
			}
		}
		
		wait(1);
	}
}