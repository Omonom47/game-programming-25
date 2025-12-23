#define TEXTURE_PIXELS_PER_UNIT 16 // how many pixels of textures will be mapped to a single world unit
#define CAMERA_PIXELS_PER_UNIT  32  // how many pixels of windows will be used to render a single world unit

#define ENABLE_DIAGNOSTICS

#define DEBUG // comment out to disable debug information

// rendering framerate
#define TARGET_FRAMERATE_NS     (SECONDS(1) / 60)

// physics timestep
#define PHYSICS_TIMESTEP_NSECS  (SECONDS(1) / 60)
#define PHYSICS_TIMESTEP_SECS   NS_TO_SECONDS(PHYSICS_TIMESTEP_NSECS)
#define PHYSICS_MAX_TIMESTEPS_PER_FRAME 4
#define PHYSICS_MAX_CONTACTS_PER_ENTITY 16

#define WINDOW_W         1024
#define WINDOW_H         576

// ui colors
#define EX6_COLOR_BTN_DEFAULT color { 0.5f, 0.5f, 0.5f, 1.0f }
#define EX6_COLOR_BTN_HOVER   color { 0.75f, 0.75f, 0.75f, 1.0f }
#define EX6_COLOR_BTN_CLICK   color { 1.0f, 1.0f, 1.0f, 1.0f }

#define COLOR_TEXT_WIN  ImVec4(0, 1, 0, 1) 
#define COLOR_TEXT_LOSE ImVec4(1, 0, 0, 1) 

#define COLOR_ENEMY_DAMAGED color{1.0f, 0.1f, 0.1f, 1.0f} 
#define COLOR_ENEMY_NORMAL  color{1.0f, 1.0f, 1.0f, 1.0f} 

// tileset information
#define TILESET_NUM_COLS 12
#define TILESET_NUM_ROWS 11

// room information
#define ROOM_NUM_TILES_X 30
#define ROOM_NUM_TILES_Y 30
#define NUM_ROOMS 5
#define MAX_ENEMIES_PER_ROOM 7
#define MIN_NUM_BULLETS 15

// audio
#define MAX_AUDIO_CHANNELS 8
#define BACKGROUND_MUSIC_VOLUME 0.8f

// Tile IDs (
#define TILE_ID_WALL 40
#define TILE_ID_FLOOR 48
#define TILE_ID_FLOOR_ALT 42

// Sprite Coordinates (Grid X, Y)
#define SPRITE_PLAYER_X 0
#define SPRITE_PLAYER_Y 8
#define SPRITE_ENEMY_X 0
#define SPRITE_ENEMY_Y 9
#define SPRITE_BULLET_X 11
#define SPRITE_BULLET_Y 10
#define SPRITE_GOAL_X 5
#define SPRITE_GOAL_Y 10

// Gameplay Balance
#define SPAWN_CHECK_INTERVAL 3.0f
#define SPAWN_DISTANCE_FROM_PLAYER_SQ (10.0f * 10.0f)



enum CollisionCategories{
    PLAYER = 0b00001,
    BULLETS = 0b00010,
    ENEMIES = 0b00100,
    WALLS = 0b01000,
    GOAL = 0b10000
};