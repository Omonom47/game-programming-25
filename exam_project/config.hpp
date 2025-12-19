#define TEXTURE_PIXELS_PER_UNIT 16 // how many pixels of textures will be mapped to a single world unit
#define CAMERA_PIXELS_PER_UNIT  32  // how many pixels of windows will be used to render a single world unit

#define ENABLE_DIAGNOSTICS

// rendering framerate
#define TARGET_FRAMERATE_NS     (SECONDS(1) / 60)

// physics timestep
#define PHYSICS_TIMESTEP_NSECS  (SECONDS(1) / 60)
#define PHYSICS_TIMESTEP_SECS   NS_TO_SECONDS(PHYSICS_TIMESTEP_NSECS)
#define PHYSICS_MAX_TIMESTEPS_PER_FRAME 4
#define PHYSICS_MAX_CONTACTS_PER_ENTITY 16

#define WINDOW_W         800
#define WINDOW_H         600

#define ENTITY_COUNT 4

// ui colors
#define EX6_COLOR_BTN_DEFAULT color { 0.5f, 0.5f, 0.5f, 1.0f }
#define EX6_COLOR_BTN_HOVER   color { 0.75f, 0.75f, 0.75f, 1.0f }
#define EX6_COLOR_BTN_CLICK   color { 1.0f, 1.0f, 1.0f, 1.0f }

// tileset information
#define TILESET_NUM_COLS 12
#define TILESET_NUM_ROWS 11

// room information

#define ROOM_NUM_TILES_X 30
#define ROOM_NUM_TILES_Y 30
#define MAX_ENEMIES_PER_ROOM 10
#define BULLET_POOL_SIZE 30

enum CollisionCategories{
    PLAYER = 0b00001,
    BULLETS = 0b00010,
    ENEMIES = 0b00100,
    WALLS = 0b01000,
    GOAL = 0b10000
};