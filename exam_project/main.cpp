#include <itu_unity_include.hpp>

// ui colors
#define COLOR_BTN_DEFAULT color{0.5f, 0.5f, 0.5f, 1.0f}
#define COLOR_BTN_HOVER color{0.75f, 0.75f, 0.75f, 1.0f}
#define COLOR_BTN_CLICK color{1.0f, 1.0f, 1.0f, 1.0f}

float design_speed_linear;
float design_speed_rotational;

enum Tags
{
	TAG_CAMERA_TARGET,
	TAG_ENEMY,
	TAG_GOAL

};

enum GameState
{
	STATE_MENU,
	STATE_PLAYING,
	STATE_GAMEOVER
};
static float survival_time = 0.0f;

static ITU_EntityId id_player;

static TTF_TextEngine *ttf_engine;

static const int tilemap_count = 5;
static ITU_EntityId tilemaps[tilemap_count];
static bool is_tilemaps_filled = false;

const int tile_mapping[] = {
	40, // wall
	48, // floor
	42, // Alternate floor
};

const float PIXELS_PER_METER = (float)TEXTURE_PIXELS_PER_UNIT;
const float METERS_PER_PIXEL = 1.0f / PIXELS_PER_METER;

// =============================================================
// 	Audio
// =============================================================

static ITU_IdAudio id_background_music;
const char *BACKGROUND_MUSIC = "../data/kenney/SFX/dungeon_ambience.ogg";

const char* FOOTSTEP_SOUND_PATHS[] = {
    "../data/kenney/SFX/footstep00.ogg",
    "../data/kenney/SFX/footstep01.ogg",
    "../data/kenney/SFX/footstep02.ogg",
    "../data/kenney/SFX/footstep03.ogg",
    "../data/kenney/SFX/footstep04.ogg",
	"../data/kenney/SFX/footstep05.ogg",
    "../data/kenney/SFX/footstep06.ogg",
    "../data/kenney/SFX/footstep07.ogg",
    "../data/kenney/SFX/footstep08.ogg",
    "../data/kenney/SFX/footstep09.ogg"
};
static ITU_IdAudio id_footsteps_sounds[std::size(FOOTSTEP_SOUND_PATHS)];
const float FOOTSTEP_INTERVAL = 0.4f;
static float footstep_timer = 0.0f;

const char* PLAYER_SOUND_PATHS[] = {
    "../data/kenney/SFX/player_impact00.ogg",
	"../data/kenney/SFX/player_impact01.ogg",
	"../data/kenney/SFX/player_impact02.ogg"
};
static ITU_IdAudio id_player_sounds[std::size(PLAYER_SOUND_PATHS)];

const char* BULLET_SOUND_PATHS[] = {
    "../data/kenney/SFX/bullet_impact00.ogg",
	"../data/kenney/SFX/bullet_impact01.ogg",
	"../data/kenney/SFX/bullet_impact02.ogg"
};
static ITU_IdAudio id_bullet_sounds[std::size(BULLET_SOUND_PATHS)];

// =============================================================
// 	Creation methods
// =============================================================

void create_goal(SDLContext *context, vec2f position)
{
	ITU_EntityId id = itu_entity_create();
	#ifdef DEBUG
	printf("Creating goal at position: (%f, %f)\n", position.x, position.y);
	itu_entity_set_debug_name(id, "goal");
	#endif

	Transform transform = TRANSFORM_DEFAULT;
	transform.position = position;

	Sprite sprite;
	SDL_Texture *texture = itu_sys_rstorage_texture_get_ptr(0);
	itu_lib_sprite_init(&sprite, texture, itu_lib_sprite_get_rect(5, 10, TEXTURE_PIXELS_PER_UNIT, TEXTURE_PIXELS_PER_UNIT));
	sprite.tint = COLOR_GREEN;

	PhysicsData physics_data = {0};
	b2BodyDef body_def = b2DefaultBodyDef();
	body_def.type = b2_staticBody;
	body_def.position = value_cast(b2Vec2, position);
	physics_data.body_id = itu_sys_physics_add_body(value_cast(void *, id), &body_def);

	b2ShapeDef shape_def = b2DefaultShapeDef();
	shape_def.isSensor = false;
	shape_def.enableContactEvents = true;
	shape_def.filter.categoryBits = GOAL;
	shape_def.filter.maskBits = PLAYER;
	b2Polygon box = b2MakeBox(0.4f, 0.4f); // TODO: change size?

	ShapeData shape_data;
	shape_data.shape_id = b2CreatePolygonShape(physics_data.body_id, &shape_def, &box);

	entity_add_component(id, Transform, transform);
	entity_add_component(id, Sprite, sprite);
	entity_add_component(id, PhysicsData, physics_data);
	entity_add_component(id, ShapeData, shape_data);
	itu_entity_tag_add(id, TAG_GOAL);
}

static std::vector<vec2f> valid_spawn_locations;
static std::vector<vec2f> room_spawn_locations[tilemap_count];
void create_enemy(SDLContext *context, vec2f position, SDL_Texture *texture)
{
	ITU_EntityId enemy_id = itu_entity_create();
	#ifdef DEBUG
    itu_entity_set_debug_name(enemy_id, "enemy");
	#endif
    Transform transform = TRANSFORM_DEFAULT;
    transform.position = position;

	Sprite sprite;
	itu_lib_sprite_init(&sprite, texture, itu_lib_sprite_get_rect(0, 9, TEXTURE_PIXELS_PER_UNIT, TEXTURE_PIXELS_PER_UNIT));

	PhysicsData physics_data = {0};
	physics_data.ignore_rotation = true;

	b2BodyDef body_def = b2DefaultBodyDef();
	body_def.userData = value_cast(void *, enemy_id);
	body_def.type = b2_dynamicBody;
	b2Circle circle = {0};
	circle.radius = 0.25f;
	body_def.position = value_cast(b2Vec2, transform.position);
	physics_data.body_id = itu_sys_physics_add_body(value_cast(void *, enemy_id), &body_def);

	b2ShapeDef shape_def = b2DefaultShapeDef();
	shape_def.enableContactEvents = true;
	shape_def.filter.categoryBits = ENEMIES;
	shape_def.filter.maskBits = PLAYER | BULLETS | WALLS;
	ShapeData shape_data;
	shape_data.shape_id = b2CreateCircleShape(physics_data.body_id, &shape_def, &circle);

    EnemyData ed = { 0 };
    ed.curr_speed_linear = 4;
	int max_health = 100;
    Health enemy_health = { max_health, max_health , 1.0f, 0.0f };

	entity_add_component(enemy_id, Transform, transform);
	entity_add_component(enemy_id, PhysicsData, physics_data);
	entity_add_component(enemy_id, ShapeData, shape_data);
	entity_add_component(enemy_id, EnemyData, ed);
	entity_add_component(enemy_id, Sprite, sprite);
	entity_add_component(enemy_id, Health, enemy_health);
	itu_entity_tag_add(enemy_id, TAG_ENEMY);
}

ITU_EntityId create_bullet(vec2f position, BulletData data)
{
	SDL_Texture *texture_tiles = itu_sys_rstorage_texture_get_ptr(0);

	b2Capsule capsule = {0};
	capsule.center1 = b2Vec2_zero;
	capsule.center2 = b2Vec2_zero;
	capsule.radius = 0.15f;
	ITU_EntityId id = itu_entity_create();
	
	#ifdef DEBUG
	itu_entity_set_debug_name(id,"bullet");
	#endif

	Transform transform = TRANSFORM_DEFAULT;
	transform.position = position;
	// Calculate bullet rotation
	float bullet_angle = atan2(data.direction.y, data.direction.x);
	transform.rotation = bullet_angle - PI_HALF;

	Sprite sprite;
	itu_lib_sprite_init(&sprite, texture_tiles, itu_lib_sprite_get_rect(11, 10, TEXTURE_PIXELS_PER_UNIT, TEXTURE_PIXELS_PER_UNIT));

	PhysicsData pd = {0};
	pd.ignore_rotation = true;
	b2BodyDef body_def = b2DefaultBodyDef();
	body_def.type = b2_dynamicBody;
	body_def.position = value_cast(b2Vec2, transform.position);
	body_def.userData = value_cast(void *, id);
	body_def.isBullet = true;
	
	pd.body_id = itu_sys_physics_add_body(value_cast(void*, id), &body_def);
	b2Body_SetUserData(pd.body_id,value_cast(void*, id));
	
	ShapeData shape_data;
	b2ShapeDef shape_def = b2DefaultShapeDef();
	shape_def.enableContactEvents = true;
	shape_def.filter.categoryBits = BULLETS;
	shape_def.filter.maskBits = ENEMIES | WALLS;
	shape_data.shape_id = b2CreateCapsuleShape(pd.body_id, &shape_def, &capsule);

	entity_add_component(id, Transform, transform);
	entity_add_component(id, Sprite, sprite);
	entity_add_component(id, BulletData, data);
	entity_add_component(id, PhysicsData, pd);
	entity_add_component(id, ShapeData, shape_data);

	return id;
}
// =============================================================
// 	
// =============================================================

int get_room_index_from_position(vec2f position)
{
	const float HALF_ROOM_WIDTH = ROOM_NUM_TILES_X / 2.0f;
	for (int i = 0; i < tilemap_count; ++i)
	{
		if (!itu_entity_is_valid(tilemaps[i]))
			continue;

		Transform *t = entity_get_data(tilemaps[i], Transform);
		if (position.x >= (t->position.x - HALF_ROOM_WIDTH) && position.x < (t->position.x + HALF_ROOM_WIDTH) &&
			position.y >= (t->position.y - HALF_ROOM_WIDTH) && position.y < (t->position.y + HALF_ROOM_WIDTH))
		{
			return i;
		}
	}
	return -1; // Not found
}

void system_maintain_enemy_population(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	static float spawn_check_timer = 0.0f;
	const float SPAWN_CHECK_INTERVAL = 3.0f;

	spawn_check_timer += context->delta;

	if(spawn_check_timer < SPAWN_CHECK_INTERVAL) return;
	spawn_check_timer = 0.0f;

	const int enemies_per_room = MAX_ENEMIES_PER_ROOM;
	const float min_spawn_distance_sq = SPAWN_DISTANCE_FROM_PLAYER * SPAWN_DISTANCE_FROM_PLAYER;

	// Count active enemies in each room
	std::vector<int> enemies_in_room(tilemap_count, 0);

	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		if (!itu_entity_is_valid(id) || !entity_get_isActive(id))
			continue;

		Transform *transform = entity_get_data(id, Transform);
		int room_idx = get_room_index_from_position(transform->position);

		if (room_idx == -1)
			continue;
		enemies_in_room[room_idx]++;
	}

	// Spawn enemies in rooms that are below the threshold
	Transform *player_transform = entity_get_data(id_player, Transform);
	vec2f player_position = player_transform->position;

	SDL_Texture *texture_enemy = itu_sys_rstorage_texture_get_ptr(0);

	for (int i = 0; i < tilemap_count; ++i)
	{
		if (enemies_in_room[i] < enemies_per_room && !room_spawn_locations[i].empty())
		{
			int rand_idx = random_up_to((int)room_spawn_locations[i].size(), context->prng);
			vec2f candidate_pos = room_spawn_locations[i][rand_idx];

			// Ensure spawn position is far enough from player
			if (distance_sq(player_position, candidate_pos) >= min_spawn_distance_sq)
			{
				create_enemy(context, candidate_pos, texture_enemy);
				return; // Spawn one enemy per update
			}
		}
	}
}

// =============================================================
// UI methods
// =============================================================

void sprite9patch_render(SDLContext *context, Sprite9Patch *sprite, TransformScreen *transform)
{
	SDL_FRect rect_src = sprite->rect;
	SDL_FRect rect_dst;

	rect_dst.w = transform->scale.x * sprite->size.x;
	rect_dst.h = transform->scale.y * sprite->size.y;
	rect_dst.x = transform->position.x - sprite->pivot.x * rect_dst.w;
	rect_dst.y = transform->position.y - sprite->pivot.y * rect_dst.h;

	rect_dst.w = SDL_max(rect_dst.w, sprite->margins_hor.x + sprite->margins_hor.y);
	rect_dst.h = SDL_max(rect_dst.h, sprite->margins_ver.x + sprite->margins_ver.y);

	SDL_FPoint pivot_dst;
	pivot_dst.x = sprite->pivot.x * rect_dst.w;
	pivot_dst.y = sprite->pivot.y * rect_dst.h;

	sdl_set_texture_tint(sprite->texture, sprite->tint);
	SDL_RenderTexture9Grid(
		context->renderer,
		sprite->texture,
		&rect_src,
		sprite->margins_hor.x,
		sprite->margins_hor.y,
		sprite->margins_ver.x,
		sprite->margins_ver.y,
		transform->scale.x,
		&rect_dst);
}

void system_sprite9patch_render(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		TransformScreen *transform = entity_get_data(id, TransformScreen);
		Sprite9Patch *sprite = entity_get_data(id, Sprite9Patch);

		sprite9patch_render(context, sprite, transform);
	}
}

void system_health(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		HealthRenderer *renderer = entity_get_data(id, HealthRenderer);
		Sprite9Patch *sprite = entity_get_data(id, Sprite9Patch);

		if(!itu_entity_is_valid(renderer->target)) continue;
		Health* health = entity_get_data(renderer->target, Health);
		sprite->size.x = renderer->widget_base_w * (health->curr / (float)health->max);
	}
}

void system_weapon_cooldown(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		CooldownRenderer *renderer = entity_get_data(id, CooldownRenderer);
		Sprite9Patch *sprite = entity_get_data(id, Sprite9Patch);

		if (!itu_entity_is_valid(renderer->target))
			continue;
		ShooterData *shooter_data = entity_get_data(renderer->target, ShooterData);
		Weapon weapon = shooter_data->weapon;
		sprite->size.x = renderer->widget_base_w * (shooter_data->cooldown_left / weapon.cooldown);
	}
}

// =============================================================
// Delete scheduled entities
// =============================================================

std::vector<std::tuple<b2BodyId, ITU_EntityId>> bodiesScheduleForDeletion;

void destroyEntitiesScheduled()
{
	std::set<int32_t> alreadyHandled;

	for (auto &body_entity_pair : bodiesScheduleForDeletion)
	{

		b2BodyId body = std::get<0>(body_entity_pair);
		ITU_EntityId entity_id = std::get<1>(body_entity_pair);

		if (auto search = alreadyHandled.find(body.index1); search != alreadyHandled.end())
		{
			continue;
		}

		alreadyHandled.insert(body.index1);

		entity_set_active(entity_id, false);
		b2DestroyBody(body);
	}
	alreadyHandled.clear();
	bodiesScheduleForDeletion.clear();
}

void free_map()
{
	if (!is_tilemaps_filled)
		return;

	for (int i = 0; i < tilemap_count; i++)
	{
		Tilemap *tilemap = entity_get_data(tilemaps[i], Tilemap);
		free(tilemap->tile_ids);
	}
	is_tilemaps_filled = false;
}

// ============================================================================================
// Map generation methods
// ============================================================================================

vec2f room_coordinate_to_world_position(int room_x, int room_y, int room_width, int room_height)
{
	float world_x = room_x * room_width;
	float world_y = room_y * room_height;
	return vec2f{world_x, world_y};
}

std::tuple<vec2f, vec2f> tile_coordinate_to_world_position(Tilemap *tilemap, Transform *transform, int tile_col, int tile_row, float tile_offset = -0.5f, vec2f pivot = vec2f{0.5f, 0.5f})
{
	float width = transform->scale.x;
	float height = transform->scale.y;

    float room_width = tilemap->num_cols * width;
    float room_height = tilemap->num_rows * height;

	// Calculate centered position
	float x = transform->position.x + (tile_col + tile_offset) * width - (pivot.x * room_width);
	float y = transform->position.y + (tile_row + tile_offset) * height - (pivot.y * room_height);

	return std::make_tuple(vec2f{width, height}, vec2f{x, y});
}

bool is_solid_tile(int tile_id)
{
	return tile_id == 0;
}

// ============================================================================================
// Enemy AI methods
// ============================================================================================

void system_enemy_ai(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		if (!entity_get_isActive(id))
			continue;

		// Enemy movement towards player
		EnemyData *enemy_data = entity_get_data(id, EnemyData);
		Transform *transform = entity_get_data(id, Transform);
		PhysicsData *physics_data = entity_get_data(id, PhysicsData);
		Transform *player_transform = entity_get_data(id_player, Transform);

		vec2f enemy_position = transform->position;
		vec2f player_position = player_transform->position;

		float distance_squared = distance_sq(player_position, enemy_position);
		float detection_radius_squared = 7.0f * 7.0f;

		if (distance_squared < detection_radius_squared && distance_squared > 0.001f)
		{
			vec2f direction = player_position - enemy_position;
			vec2f normalized_direction = normalize(direction);
			physics_data->velocity = normalized_direction * enemy_data->curr_speed_linear;
		}
		else
		{
			physics_data->velocity = vec2f{0, 0};
		}
	}
}

// ============================================================================================
// Collision Callbacks
// ============================================================================================

void system_player_collision_events(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];

		ShapeData *shape_data = entity_get_data(id, ShapeData);
		b2ShapeId shape_id = shape_data->shape_id;

		b2ContactData contactData[10];
		int contact = b2Shape_GetContactData(shape_id, contactData, 10);
		for (int j = 0; j < contact; ++j)
		{
			b2ContactData contact_data = contactData[j];
			b2ShapeId other_id = (contact_data.shapeIdA.index1 == shape_id.index1) ? contact_data.shapeIdB : contact_data.shapeIdA;
			b2Filter filter = b2Shape_GetFilter(other_id);

			// Handle collision with goal
			if (filter.categoryBits & GOAL)
			{
				context->game_over = true;
				return;
			}

			// Handle collision with enemies
			if (filter.categoryBits & ENEMIES)
			{
				Health *health = entity_get_data(id, Health);
				if (health->elapsed > health->grace_period)
				{
					health->curr -= 1;
					health->elapsed = 0.0f;

					int rand_index = random_up_to(std::size(id_player_sounds), context->prng);
					MIX_Audio *player_sfx = itu_sys_rstorage_audio_get_ptr(id_player_sounds[rand_index]);
					float random_volume = 0.2f + (random_up_to(30, context->prng) / 100.0f);
					sys_audio_play_sfx_gain(player_sfx, random_volume);


					if (health->curr <= 0.0f)
					{
						context->game_over = true;
					}
					return;
				}
			}
		}

	}
}

void system_bullet_collision_events(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		if (!entity_get_isActive(id))
			continue;

		ShapeData *shape_data = entity_get_data(id, ShapeData);
		b2ShapeId bullet_id = shape_data->shape_id;

		b2ContactData contactData[10];
		int contact = b2Shape_GetContactData(bullet_id, contactData, 10);
		if (contact > 0)
		{
			int rand_index = random_up_to(std::size(id_bullet_sounds), context->prng);
			MIX_Audio *bullet_sfx = itu_sys_rstorage_audio_get_ptr(id_bullet_sounds[rand_index]); 
			float random_volume = 0.2f + (random_up_to(30, context->prng) / 100.0f);
			sys_audio_play_sfx_gain(bullet_sfx, random_volume);

			entity_set_active(id, false);
			PhysicsData *phys = entity_get_data(id, PhysicsData);
			bodiesScheduleForDeletion.push_back(std::tie(phys->body_id, id));
		}
	}
}

void system_enemy_collision_events(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		if (!entity_get_isActive(id))
			continue;

		ShapeData *shape_data = entity_get_data(id, ShapeData);
		b2ShapeId enemy_id = shape_data->shape_id;

		b2ContactData contactData[10];
		int contact = b2Shape_GetContactData(enemy_id, contactData, 10);
		for (int j = 0; j < contact; ++j)
		{
			b2ContactData contact_data = contactData[j];
			b2ShapeId other_id = (contact_data.shapeIdA.index1 == enemy_id.index1) ? contact_data.shapeIdB : contact_data.shapeIdA;

			b2Filter filter = b2Shape_GetFilter(other_id);

			// Handle collision with bullets
			if (filter.categoryBits & BULLETS) {
				Health* enemy_health = entity_get_data(id, Health);
				void* user_data = b2Body_GetUserData(b2Shape_GetBody(other_id));
				ITU_EntityId bullet_id = value_cast(ITU_EntityId,user_data);
				BulletData* bullet = entity_get_data(bullet_id,BulletData);

				enemy_health->curr -= bullet->damage;
				enemy_health->elapsed = 0.0f;

				if (enemy_health->curr <= 0.0f)
				{
					PhysicsData *enemy_phys = entity_get_data(id, PhysicsData);

					bodiesScheduleForDeletion.push_back(std::tie(enemy_phys->body_id, id));
				}
			}
		}
	}
}

// ============================================================================================
// TMP methods
// ============================================================================================

void render_background_void(SDLContext *context)
{
	SDL_Texture *texture = itu_sys_rstorage_texture_get_ptr(0);
	int wall_tile_id = tile_mapping[0];
	int tile_size = PIXELS_PER_METER;

	int tile_coord_x = wall_tile_id % TILESET_NUM_COLS;
	int tile_coord_y = wall_tile_id / TILESET_NUM_COLS;

	SDL_FRect rect_src;
	rect_src.w = tile_size;
	rect_src.h = tile_size;
	rect_src.x = tile_coord_x * rect_src.w;
	rect_src.y = tile_coord_y * rect_src.h;

	// Calculate number of units fit on screen
	float view_width = context->camera_active->normalized_screen_size.x / context->camera_active->zoom * context->window_w / context->camera_active->pixels_per_unit;
	float view_height = context->camera_active->normalized_screen_size.y / context->camera_active->zoom * context->window_h / context->camera_active->pixels_per_unit;

	vec2f camera_world_pos = context->camera_active->world_position;

	// Calculate top left corner of the view in world coordinates
	float start_x = camera_world_pos.x - view_width / 2.0f;
	float start_y = camera_world_pos.y - view_height / 2.0f;

	// Determine the bounds for tile rendering
	int start_tile_x = (int)floor(start_x) - 1;
	int end_tile_x = (int)ceil(start_x + view_width) + 1;

	int start_tile_y = (int)floor(start_y) - 1;
	int end_tile_y = (int)ceil(start_y + view_height) + 1;

	// Render tiles within the calculated bounds
	for (int y = start_tile_y; y < end_tile_y; ++y)
	{
		for (int x = start_tile_x; x < end_tile_x; ++x)
		{
			// get destination rect based on current x and y
			SDL_FRect rect_dst;
			rect_dst.w = 1.0f;
			rect_dst.h = 1.0f;
			rect_dst.x = (float)x - 0.5f;
			rect_dst.y = (float)y - 0.5f;
			rect_dst = rect_global_to_screen(context, rect_dst);

			// render tile
			SDL_RenderTexture(context->renderer, texture, &rect_src, &rect_dst);
		}
	}
}

void system_tilemap_render(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		Tilemap *tilemap = entity_get_data(id, Tilemap);
		Transform *transform = entity_get_data(id, Transform);
		float tile_offset = -0.5f; // to center the tile on its position

		for (int y = 0; y < tilemap->num_rows; ++y)
		{
			for (int x = 0; x < tilemap->num_cols; ++x)
			{
				int tile_id_map = tilemap->tile_ids[y * tilemap->num_cols + x];
				int tile_id_texture = tile_mapping[tile_id_map];
				int tile_coord_x = tile_id_texture % TILESET_NUM_COLS;
				int tile_coord_y = tile_id_texture / TILESET_NUM_COLS;

				// get source rect from texture size and tile coords
				SDL_FRect rect_src;
				rect_src.w = tilemap->tile_size;
				rect_src.h = tilemap->tile_size;
				rect_src.x = tile_coord_x * rect_src.w;
				rect_src.y = tile_coord_y * rect_src.h;

				// get destination rect based on current x and y
				SDL_FRect rect_dst;
				vec2f width_height;
				vec2f position;
				std::tie(width_height, position) = tile_coordinate_to_world_position(tilemap, transform, x, y);
				rect_dst.w = width_height.x;
				rect_dst.h = width_height.y;
				rect_dst.x = position.x;
				rect_dst.y = position.y;
				rect_dst = rect_global_to_screen(context, rect_dst);

				// render tile
				SDL_RenderTexture(context->renderer, tilemap->texture, &rect_src, &rect_dst);
			}
		}
	}
}

float lerp_smooth(float a, float b, float f)
{
	return a + f * (b - a);
}

void system_camera_target(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		Transform *transform = entity_get_data(id, Transform);

		// Lerp camera position to target position
		float lerp_factor = 15.0f * context->delta;

		vec2f current_camera_pos = context->camera_active->world_position;
		vec2f target_camera_pos = transform->position;

		vec2f new_pos;
		new_pos.x = lerp_smooth(current_camera_pos.x, target_camera_pos.x, lerp_factor);
		new_pos.y = lerp_smooth(current_camera_pos.y, target_camera_pos.y, lerp_factor);
		context->camera_active->world_position = new_pos;
	}
}

void lib_sprite_render_camera(SDLContext *context, Sprite *sprite, TransformScreen *transform)
{
	SDL_FRect rect_src = sprite->rect;
	SDL_FRect rect_dst;

	rect_dst.w = transform->scale.x * rect_src.w;
	rect_dst.h = transform->scale.y * rect_src.h;
	rect_dst.x = transform->position.x - sprite->pivot.x * rect_dst.w;
	rect_dst.y = transform->position.y - sprite->pivot.y * rect_dst.h;

	SDL_FPoint pivot_dst;
	pivot_dst.x = sprite->pivot.x * rect_dst.w;
	pivot_dst.y = sprite->pivot.y * rect_dst.h;

	sdl_set_texture_tint(sprite->texture, sprite->tint);
	SDL_RenderTextureRotated(
		context->renderer,
		sprite->texture,
		&rect_src,
		&rect_dst,
		(-transform->rotation) * RAD_2_DEG,
		&pivot_dst,
		sprite->flip_horizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void system_sprite_render_camera(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		TransformScreen *transform = entity_get_data(id, TransformScreen);
		Sprite *sprite = entity_get_data(id, Sprite);

		lib_sprite_render_camera(context, sprite, transform);
	}

	// outline render target
	sdl_set_render_draw_color(context, {1, 0, 1, 1});
	SDL_RenderRect(context->renderer, NULL);
}

#ifdef DEBUG
// ============================================================================================
// COMPONENT DEBUG UI RENDER methods
// ============================================================================================

void debug_ui_render_playerdata(SDLContext *context, void *data)
{
	PlayerData *data_player = (PlayerData *)data;

	ImGui::DragFloat("curr. linear speed", &data_player->curr_speed_linear);
	ImGui::DragFloat("curr. rotational speed", &data_player->curr_speed_rotational);

	itu_debug_ui_widget_entityid("target", data_player->target);
}

void debug_ui_render_health(SDLContext *context, void *data)
{
	Health *data_health = (Health *)data;

	ImGui::DragInt("max", &data_health->max);
	ImGui::DragInt("curr", &data_health->curr, 1, 0, data_health->max);
}

void debug_ui_render_healthrenderer(SDLContext *context, void *data)
{
	HealthRenderer *data_renderer = (HealthRenderer *)data;

	itu_debug_ui_widget_entityid("target", data_renderer->target);
	ImGui::DragFloat("base widget width", &data_renderer->widget_base_w);
}

void debug_ui_render_transformscreen(SDLContext *context, void *data)
{
	TransformScreen *data_transform = (TransformScreen *)data;

	ImGui::DragFloat2("position", &data_transform->position.x);
	ImGui::DragFloat2("scale", &data_transform->scale.x);

	float rotation_deg = data_transform->rotation * RAD_2_DEG;
	if (ImGui::DragFloat("rotation", &rotation_deg))
		data_transform->rotation = rotation_deg * DEG_2_RAD;

	itu_lib_render_draw_point(context->renderer, data_transform->position, 5, COLOR_YELLOW);
}

void debug_ui_render_sprite9patch(SDLContext *context, void *data)
{
	Sprite9Patch *data_sprite = (Sprite9Patch *)data;

	itu_sys_rstorage_debug_render_texture(data_sprite->texture, &data_sprite->texture, &data_sprite->rect);

	ImGui::DragFloat4("texture rect", &data_sprite->rect.x);
	ImGui::DragFloat2("pivot", &data_sprite->pivot.x);

	ImGui::DragFloat2("size", &data_sprite->size.x);
	ImGui::DragFloat2("margins ver.", &data_sprite->margins_hor.x);
	ImGui::DragFloat2("margins hor.", &data_sprite->margins_ver.x);

	ImGui::ColorEdit4("tint", &data_sprite->tint.r);
}

void debug_ui_render_imagebutton(SDLContext *context, void *data)
{
	ImageButton *data_imagebutton = (ImageButton *)data;
	// char* buf;
	//
	// TTF_SetTextString
	// ImGui::InputTextMultiline("text", buf, 1024);
	ImGui::LabelText("hover callback", "%p", data_imagebutton->fn_callback_hover);
	ImGui::LabelText("click callback", "%p", data_imagebutton->fn_callback_click);

	int wrap_width;
	TTF_GetTextWrapWidth(data_imagebutton->ttf_text, &wrap_width);

	int size[2];
	TTF_GetTextSize(data_imagebutton->ttf_text, &size[0], &size[1]);

	color c;
	TTF_GetTextColorFloat(data_imagebutton->ttf_text, &c.r, &c.g, &c.b, &c.a);

	TTF_Font *font = TTF_GetTextFont(data_imagebutton->ttf_text);
	TTF_Font *new_font;
	if (itu_sys_rstorage_debug_render_font(font, &new_font))
		TTF_SetTextFont(data_imagebutton->ttf_text, new_font);

	ImGui::InputInt2("size (readonly)", size);

	if (ImGui::DragInt("wrap width", &wrap_width))
		TTF_SetTextWrapWidth(data_imagebutton->ttf_text, wrap_width);

	if (ImGui::ColorEdit4("color", &c.r))
		TTF_SetTextColorFloat(data_imagebutton->ttf_text, c.r, c.g, c.b, c.a);
}

// ============================================================================================
//
// ============================================================================================
#endif

void system_player_update(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		Transform *transform = entity_get_data(id, Transform);
		PlayerData *data = entity_get_data(id, PlayerData);
		PhysicsData *physics_data = entity_get_data(id, PhysicsData);

		// Movement
		vec2f move_dir = VEC2F_ZERO;
		if (context->btn_isdown[BTN_TYPE_W])
			move_dir.y += 1;
		if (context->btn_isdown[BTN_TYPE_S])
			move_dir.y -= 1;
		if (context->btn_isdown[BTN_TYPE_A])
			move_dir.x -= 1;
		if (context->btn_isdown[BTN_TYPE_D])
			move_dir.x += 1;

		physics_data->velocity = normalize(move_dir) * 5;

		// Footstep sound
		bool is_moving = (move_dir.x != 0 || move_dir.y != 0);
		if (is_moving)
		{
			footstep_timer -= context->delta;
			if (footstep_timer <= 0.0f)
			{
				footstep_timer = FOOTSTEP_INTERVAL;

				// Player random footstep sound
				int rand_index = random_up_to(std::size(id_footsteps_sounds), context->prng);
				MIX_Audio *footstep_sfx = itu_sys_rstorage_audio_get_ptr(id_footsteps_sounds[rand_index]);
				float random_volume = 0.1f + (random_up_to(30, context->prng) / 100.0f);
				sys_audio_play_sfx_gain(footstep_sfx, random_volume);
			}
		}
		else
		{
			footstep_timer = 0.0f;
		}

		// Shooting
		vec2f shoot_dir = VEC2F_ZERO;
		if (context->btn_isdown[BTN_TYPE_UP])
			shoot_dir = VEC2F_UP;
		if (context->btn_isdown[BTN_TYPE_DOWN])
			shoot_dir = VEC2F_DOWN;
		if (context->btn_isdown[BTN_TYPE_LEFT])
			shoot_dir = VEC2F_LEFT;
		if (context->btn_isdown[BTN_TYPE_RIGHT])
			shoot_dir = VEC2F_RIGHT;

		if (shoot_dir.x != 0 || shoot_dir.y != 0)
			data->rotation = normalize(shoot_dir);
		else if (move_dir.x != 0 || move_dir.y != 0)
			data->rotation = normalize(move_dir);

		const float step = PI / 2.0f;
		float angle = atan2(data->rotation.y, data->rotation.x);
		float snapped_angle = round(angle / step) * step;
		transform->rotation = snapped_angle - PI_HALF;
	}
}

void system_health_update(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		Health *health = entity_get_data(id, Health);
		Sprite *sprite = entity_get_data(id, Sprite);

		// Update elapsed time
		health->elapsed += context->delta;

		float flash_duration = 0.25f;
		if (health->elapsed < flash_duration)
		{
			sprite->tint = color{1.0f, 0.1f, 0.1f, 1.0f}; // Red
		}
		else
		{
			sprite->tint = color{1.0f, 1.0f, 1.0f, 1.0f}; // Normal
		}
	}
}

float calculate_max_angle(int bullet_amount){
	int ba_sqrd = bullet_amount*bullet_amount;

	float angle = ba_sqrd*bullet_amount;
	angle /= 8.0f;
	angle -= ba_sqrd/4.0f;
	angle += bullet_amount << 3;
	angle += 15.0f;
	return angle;
}

void set_bullet_dirs(ShotPattern pattern, int bullet_amount, vec2f direction, vec2f* out){
	
	switch (pattern)
		{
			case SPREAD:
				float max_angle; 
				max_angle = DEG_2_RAD *calculate_max_angle(bullet_amount);
				float angle_between_shots;
				angle_between_shots = max_angle/(float)(bullet_amount-1);
				float angle;
				angle = max_angle/2.0f;
				int idx;
				idx = 0;
				if (is_odd(bullet_amount))
				{
					out[idx++] = direction;
				}
				
				while (idx < bullet_amount)
				{
					vec2f new_dir = rotate(direction,angle);
					
					vec2f reflected = reflect(-new_dir,direction);	

					out[idx++] = normalize(new_dir);
					out[idx++] = normalize(reflected);

					angle -= angle_between_shots;
				}
				
			break;
		
		default:
			break;
		}
}

void system_player_shooting(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; i++)
	{
		ITU_EntityId id = entity_ids[i];
		ShooterData *shooter = entity_get_data(id, ShooterData);
		PlayerData *player_data = entity_get_data(id, PlayerData);

		if (shooter->cooldown_left >= 0)
		{
			shooter->cooldown_left -= context->delta;
		}

		bool canShoot = shooter->cooldown_left <= 0;

		if (!context->btn_isjustpressed_space || !canShoot)
			continue;

		Transform *transform = entity_get_data(id, Transform);

		ITU_EntityId null_ent = ITU_ENTITY_ID_NULL;
		vec2f direction = player_data->rotation;

		int bullet_amount = shooter->weapon.bullets_per_shot;

		vec2f dirs[bullet_amount];

		set_bullet_dirs(shooter->weapon.pattern, bullet_amount, direction, dirs);

		for (int j = 0; j < bullet_amount; j++)
		{
			BulletData bd;
			bd.damage = shooter->weapon.damage;
			bd.direction = shooter->weapon.pattern == SPREAD ? dirs[j] : direction;
			bd.speed = shooter->weapon.bullet_speed;
			bd.update_behaviour = shooter->weapon.fn_bullet_behaviour;

			ITU_EntityId bullet_id = create_bullet(transform->position, bd);
		}
		shooter->cooldown_left = shooter->weapon.cooldown;
	}
}

void system_bullet_update(SDLContext *context, ITU_EntityId *entity_ids, int entity_ids_count)
{
	for (int i = 0; i < entity_ids_count; i++)
	{
		ITU_EntityId id = entity_ids[i];

		if (!entity_get_isActive(id))
			continue;

		BulletData *bd = entity_get_data(id, BulletData);
		PhysicsData *pd = entity_get_data(id, PhysicsData);

		bd->update_behaviour(pd, bd->direction, bd->speed);
	}
}

static void game_init(SDLContext *context)
{

	itu_sys_rstorage_texture_load(context, "data/kenney/tiny_dungeon_packed.png", SDL_SCALEMODE_NEAREST);
	itu_sys_rstorage_texture_load(context, "data/kenney/UI/bar_round_gloss_small_red.png", SDL_SCALEMODE_LINEAR);
	itu_sys_rstorage_font_load(context, "data/ARIALBD.TTF", 42);

	sys_audio_init(32);
	sys_audio_set_gain_music(0.8f); // Set volume

	id_background_music = itu_sys_rstorage_audio_load(context, BACKGROUND_MUSIC, true);;

	for (int i = 0; i < std::size(FOOTSTEP_SOUND_PATHS); ++i) {
        id_footsteps_sounds[i] = itu_sys_rstorage_audio_load(context, FOOTSTEP_SOUND_PATHS[i], false);
    }

	for (int i = 0; i < std::size(PLAYER_SOUND_PATHS); ++i) {
        id_player_sounds[i] = itu_sys_rstorage_audio_load(context, PLAYER_SOUND_PATHS[i], false);
    }

	for (int i = 0; i < std::size(BULLET_SOUND_PATHS); ++i) {
        id_bullet_sounds[i] = itu_sys_rstorage_audio_load(context, BULLET_SOUND_PATHS[i], false);
    }
	
	itu_sys_estorage_init(512);
	itu_sys_physics_init(context);

	enable_component(Tilemap);
	enable_component(PlayerData);
	enable_component(TransformScreen);
	enable_component(EnemyData);
	enable_component(BulletData);
	enable_component(ShooterData);
	enable_component(Health);
	enable_component(Sprite9Patch);
	enable_component(HealthRenderer);
	enable_component(CooldownRenderer);
	
	#ifdef DEBUG
	add_component_debug_ui_render(PlayerData, debug_ui_render_playerdata);
	add_component_debug_ui_render(TransformScreen, debug_ui_render_transformscreen);
	//TODO: add_component_debug_ui_render(Tilemap, debug_ui_render_tilemap);
	#endif

	itu_sys_estorage_tag_set_debug_name(TAG_CAMERA_TARGET, "camera target");
	itu_sys_estorage_tag_set_debug_name(TAG_ENEMY, "enemy");
	itu_sys_estorage_tag_set_debug_name(TAG_GOAL, "Goal");

	add_system(system_tilemap_render, component_mask(Transform) | component_mask(Tilemap), 0, true);
	add_system(system_player_update, component_mask(Transform) | component_mask(PhysicsData) | component_mask(PlayerData), 0, false);
	add_system(system_camera_target, component_mask(Transform), tag_mask(TAG_CAMERA_TARGET), false);

	add_system(system_player_shooting, component_mask(PlayerData) | component_mask(ShooterData) | component_mask(Transform), 0, false);
	add_system(system_bullet_update, component_mask(BulletData) | component_mask(PhysicsData) | component_mask(Transform), 0, false);
	add_system(itu_system_sprite_render, component_mask(Transform) | component_mask(Sprite), 0, true);

	add_system(system_player_collision_events, component_mask(PlayerData) | component_mask(ShapeData), 0, false);
	add_system(system_enemy_collision_events, component_mask(EnemyData) | component_mask(ShapeData), 0, false);
	add_system(system_bullet_collision_events, component_mask(BulletData) | component_mask(ShapeData), 0, false);

	add_system(system_enemy_ai, component_mask(Transform) | component_mask(EnemyData), tag_mask(TAG_ENEMY), false);
	add_system(system_health, component_mask(TransformScreen) | component_mask(Sprite9Patch) | component_mask(HealthRenderer), 0, false);
	add_system(system_weapon_cooldown, component_mask(TransformScreen) | component_mask(Sprite9Patch) | component_mask(CooldownRenderer), 0, false);
	add_system(system_sprite9patch_render, component_mask(TransformScreen) | component_mask(Sprite9Patch), 0, true);
	add_system(system_maintain_enemy_population, component_mask(Transform) | component_mask(EnemyData), tag_mask(TAG_ENEMY), false);
	add_system(system_health_update, component_mask(Health) | component_mask(Sprite), 0, false);
}

static void game_reset(SDLContext *context)
{
	// TMP get textures pointers
	SDL_Texture *texture_tiles = itu_sys_rstorage_texture_get_ptr(0);
	SDL_Texture *texture_healthbar = itu_sys_rstorage_texture_get_ptr(1);
	TTF_Font *font_bold = itu_sys_rstorage_font_get_ptr(0);

	free_map();
	itu_sys_estorage_clear_all_entities();

	b2WorldDef world_def = b2DefaultWorldDef();
	world_def.gravity.y = 0; // Since top-down there is no gravity (anything falling downwards)
	itu_sys_physics_reset(&world_def);

	//tilemaps
	{
		// Clear previous valid spawn locations
		valid_spawn_locations.clear();
		valid_spawn_locations.shrink_to_fit();
		Tilemap rooms[tilemap_count];
		Map map = generate_map(rooms, 4, 4, tilemap_count, context->prng);

		for (int i = 0; i < tilemap_count; ++i)
		{
			room_spawn_locations[i].clear();
			room_spawn_locations[i].shrink_to_fit();
			Point location = map.room_locations[i];

			ITU_EntityId id_tilemap = itu_entity_create();
			tilemaps[i] = id_tilemap;
			#ifdef DEBUG
			itu_entity_set_debug_name(id_tilemap, "tilemap");
			#endif
			rooms[i].texture = texture_tiles;
			rooms[i].tile_size = 16;
			rooms[i].pivot = {0.5f, 0.5f};

			Transform transform = TRANSFORM_DEFAULT;
			transform.scale = vec2f{1.0f, 1.0f};
			transform.position = room_coordinate_to_world_position(location.x, location.y, map.room_rows, map.room_cols);

			entity_add_component(id_tilemap, Transform, transform);
			entity_add_component(id_tilemap, Tilemap, rooms[i]);
		}

		int cols = map.room_cols;
		int rows = map.room_rows;

		for (int i = 0; i < tilemap_count; i++)
		{
			// player vs tile collision detection
			for (int r = 0; r < rows; ++r)
			{
				for (int c = 0; c < cols; ++c)
				{
					int idx = r * cols + c;
					Tilemap *tilemap = entity_get_data(tilemaps[i], Tilemap);
					Transform *transform = entity_get_data(tilemaps[i], Transform);
					int tile_id = tilemap->tile_ids[idx];

					vec2f tile_position;
					std::tie(std::ignore, tile_position) = tile_coordinate_to_world_position(tilemap, transform, c, r, 0);
					if (!is_solid_tile(tile_id))
					{
						valid_spawn_locations.push_back(tile_position);
						room_spawn_locations[i].push_back(tile_position);
					}
					else
					{
						// Create colliders by merging horizontal tiles
						int width = 1;
						while (c + width < cols && is_solid_tile(tilemap->tile_ids[idx + width]))
							++width;

						ITU_EntityId row_id = itu_entity_create();
						#ifdef DEBUG
						itu_entity_set_debug_name(row_id, "tile-collider");
						#endif

						b2BodyDef tile_body_def = b2DefaultBodyDef();
						tile_body_def.userData = value_cast(void *, row_id);
						tile_body_def.type = b2_staticBody;

						// Calculate center of rectangle
						float row_end_x = tile_position.x + width;
						tile_body_def.position = b2Vec2{(tile_position.x + row_end_x) * 0.5f - 0.5f, tile_position.y};

						b2BodyId body_id = itu_sys_physics_add_body(value_cast(void *, row_id), &tile_body_def);
						b2ShapeDef shape_def = b2DefaultShapeDef();
						shape_def.enableContactEvents = false;
						shape_def.filter.categoryBits = WALLS;
						b2Polygon box = b2MakeBox(width * 0.5f, 0.5f);

						ShapeData shape_data = {0};
						shape_data.shape_id = b2CreatePolygonShape(body_id, &shape_def, &box);
						entity_add_component(row_id, ShapeData, shape_data);

						// Skip consumed
						c += (width - 1);
					}
				}
			}
		}

		is_tilemaps_filled = true;

		// Place player on random valid spawn location
		if (!valid_spawn_locations.empty())
		{
			int random_idx = random_up_to((int)valid_spawn_locations.size(), context->prng);
			context->player_start_position = valid_spawn_locations[random_idx];
		}

		// Goal post placement (farthest point from player)
		if (!valid_spawn_locations.empty())
		{
			vec2f goal_position = VEC2F_ZERO;
			float max_dist_sq = -1.0f;

			for (const auto &pos : valid_spawn_locations)
			{
				float distance = distance_sq(context->player_start_position, pos);
				if (distance > max_dist_sq)
				{
					max_dist_sq = distance;
					goal_position = pos;
				}
			}
			create_goal(context, goal_position);
		}
	}

	// player
	{
		id_player = itu_entity_create();
		#ifdef DEBUG
		itu_entity_set_debug_name(id_player, "player");
		#endif
		Transform transform = TRANSFORM_DEFAULT;
		transform.position = context->player_start_position;

		Sprite sprite;
		itu_lib_sprite_init(&sprite, texture_tiles, itu_lib_sprite_get_rect(0, 8, TEXTURE_PIXELS_PER_UNIT, TEXTURE_PIXELS_PER_UNIT));

		PlayerData data = {0};
		data.rotation = vec2f{0.0f, 1.0f};

		PhysicsData physics_data = {0};
		physics_data.ignore_rotation = true;

		b2BodyDef body_def = b2DefaultBodyDef();
		body_def.userData = value_cast(void *, id_player);
		body_def.type = b2_dynamicBody;
		body_def.position = value_cast(b2Vec2, transform.position);
		physics_data.body_id = itu_sys_physics_add_body(value_cast(void *, id_player), &body_def);

		b2ShapeDef shape_def = b2DefaultShapeDef();
		shape_def.enableContactEvents = true;
		shape_def.filter.categoryBits = PLAYER;
		shape_def.filter.maskBits = ENEMIES | WALLS | GOAL;
		b2Circle circle = {0};
		circle.radius = 0.25f;
		shape_def.isSensor = false;

		ShapeData shape_data;
		shape_data.shape_id = b2CreateCircleShape(physics_data.body_id, &shape_def, &circle);

		Health player_health = { 10, 10, 1, 1 }; //max, current, elapsed, grace_period
		
		ShooterData shooter;
		shooter.weapon = generate_weapon(context->prng);
		shooter.cooldown_left = 0;
		
		entity_add_component(id_player, Transform     , transform);
		entity_add_component(id_player, Sprite        , sprite);
		entity_add_component(id_player, PlayerData, data);
		entity_add_component(id_player, PhysicsData   , physics_data);
		entity_add_component(id_player, ShapeData     , shape_data);
		entity_add_component(id_player, Health        , player_health);
		entity_add_component(id_player,ShooterData,shooter);
		itu_entity_tag_add(id_player, TAG_CAMERA_TARGET);

		// Set camera to player
		if (context->camera_active)
		{
			context->camera_active->world_position = transform.position;
		}
	}

	// Enemies
	{
		SDL_Texture* texture = itu_sys_rstorage_texture_get_ptr(0);
		
		for(int i = 0; i < tilemap_count; ++i) {
			if(room_spawn_locations[i].empty()) continue;

			for(int count = 0; count < MAX_ENEMIES_PER_ROOM; ++count){
				int rand_idx = random_up_to((int)room_spawn_locations[i].size(), context->prng);
				vec2f candidate_position = room_spawn_locations[i][rand_idx];

				const float min_spawn_distance_sq = 5.0f * 5.0f;
				if(distance_sq(context->player_start_position, candidate_position) > min_spawn_distance_sq) {
					create_enemy(context, candidate_position, texture);
				}
			}
		}
	}

	// healthbar
	{
		ITU_EntityId id = itu_entity_create();
		#ifdef DEBUG
		itu_entity_set_debug_name(id, "Player-Healthbar");
		#endif
		TransformScreen transform = { 0 };
		transform.scale = VEC2F_ONE;
		transform.position = {20, 18};

		Sprite9Patch sprite;
		sprite.rect = {0, 0, 96, 16};
		sprite.texture = texture_healthbar;
		sprite.size = {760, 16};
		sprite.margins_hor = {8, 8};
		sprite.margins_ver = {8, 8};
		sprite.pivot.x = 0.0f;
		sprite.pivot.y = 0.0f;
		sprite.tint = COLOR_RED;

		HealthRenderer renderer;
		renderer.target = id_player;
		renderer.widget_base_w = sprite.size.x;

		entity_add_component(id, TransformScreen, transform);
		entity_add_component(id, Sprite9Patch, sprite);
		entity_add_component(id, HealthRenderer, renderer);
	}

	// weapon cooldown
	{
		ITU_EntityId id = itu_entity_create();
		#ifdef DEBUG
		itu_entity_set_debug_name(id, "Weapon-Cooldown");
		#endif
		TransformScreen transform = { 0 };
		transform.scale = VEC2F_ONE;
		transform.position = {20, 40};

		Sprite9Patch sprite;
		sprite.rect = {0, 0, 96, 16};
		sprite.texture = texture_healthbar;
		sprite.size = {190, 16};
		sprite.margins_hor = {8, 8};
		sprite.margins_ver = {8, 8};
		sprite.pivot.x = 0.0f;
		sprite.pivot.y = 0.0f;
		sprite.tint = COLOR_GREEN;

		CooldownRenderer renderer;
		renderer.target = id_player;
		renderer.widget_base_w = sprite.size.x;

		entity_add_component(id, TransformScreen, transform);
		entity_add_component(id, Sprite9Patch, sprite);
		entity_add_component(id, CooldownRenderer, renderer);
	}
}

// =============================================================
// Menu UI
// =============================================================

void render_main_menu_ui(SDLContext *context, GameState *current_state, float *survival_time, bool *quit)
{
	// Center the menu
	ImGui::SetNextWindowPos(ImVec2(context->window_w / 2.0f, context->window_h / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	if (ImGui::Begin("Main Menu", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::Text("Welcome to the never ending dungeon!");
		ImGui::Spacing();
		if (ImGui::Button("Start Game", ImVec2(200, 50)))
		{
			game_reset(context);

			MIX_Audio *music_ptr = itu_sys_rstorage_audio_get_ptr(id_background_music);
			if (music_ptr)
				sys_audio_play_music(music_ptr, -1); // Loop indefinitely

			*survival_time = 0.0f;
			*current_state = STATE_PLAYING;
		}
		ImGui::Spacing();
		if (ImGui::Button("Quit", ImVec2(200, 50)))
		{
			*quit = true;
		}
		ImGui::End();
	}
}

void render_game_over_ui(SDLContext *context, GameState *current_state, float *survival_time, bool *quit)
{
	ImGui::SetNextWindowPos(ImVec2(context->window_w / 2.0f, context->window_h / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	if (ImGui::Begin("Game Over", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "GAME OVER");
		ImGui::Spacing();

		ImGui::Text("You survived for %.2f seconds!", *survival_time);
		ImGui::Spacing();

		if (ImGui::Button("Retry", ImVec2(200, 50)))
		{
			game_reset(context);
			*survival_time = 0.0f;
			*current_state = STATE_PLAYING;
		}

		if (ImGui::Button("Return to Main Menu", ImVec2(200, 50)))
		{
			*current_state = STATE_MENU;
		}
		ImGui::End();
	}
}

void gameplay_loop(SDLContext *context, GameState *current_state, float *round_timer)
{
	*round_timer += context->delta;

	// Render and update game systems
	render_background_void(context);
	itu_sys_estorage_systems_update(context);

	if (context->render_debug)
	{
		itu_sys_physics_debug_draw();
	}

	// Check for game over
	if (context->game_over)
	{
		*current_state = STATE_GAMEOVER;
		context->game_over = false;
		sys_audio_stop_music(NULL, 1000);
	}
}

int main(void)
{
	bool quit = false;
	SDLContext context = {0};

	PRNG engine;
	init_rng(&engine);
	context.prng = &engine;

	context.window_w = WINDOW_W;
	context.window_h = WINDOW_H;
	context.render_debug = false;

	TTF_Init();

	context.working_dir = SDL_GetCurrentDirectory();
	SDL_CreateWindowAndRenderer("Not Binding of Isaac", WINDOW_W, WINDOW_H, 0, &context.window, &context.renderer);
	SDL_SetRenderDrawBlendMode(context.renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderVSync(context.renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

	// increase the zoom to make debug text more legible
	// (ie, on the class projector, we will usually use 2)
	{
		context.zoom = 1;
		context.window_w /= context.zoom;
		context.window_h /= context.zoom;
		SDL_SetRenderScale(context.renderer, context.zoom, context.zoom);
	}

	itu_lib_imgui_setup(context.window, &context, true);

	context.camera_default.normalized_screen_size.x = 1.0f;
	context.camera_default.normalized_screen_size.y = 1.0f;
	context.camera_default.normalized_screen_offset.x = 0.0f;
	context.camera_default.zoom = 1;
	context.camera_default.pixels_per_unit = CAMERA_PIXELS_PER_UNIT;
	camera_set_active(&context, &context.camera_default);

	context.debug_ui_show = true;

	game_init(&context);

	SDL_Time walltime_frame_beg;
	SDL_Time walltime_frame_end;
	SDL_Time walltime_work_end;
	SDL_Time elapsed_work = 0;
	SDL_Time elapsed_frame = 0;

	SDL_GetCurrentTime(&walltime_frame_beg);
	walltime_frame_end = walltime_frame_beg;

	sdl_input_set_mapping_keyboard(&context, SDLK_W, BTN_TYPE_W);
	sdl_input_set_mapping_keyboard(&context, SDLK_A, BTN_TYPE_A);
	sdl_input_set_mapping_keyboard(&context, SDLK_S, BTN_TYPE_S);
	sdl_input_set_mapping_keyboard(&context, SDLK_D, BTN_TYPE_D);
	sdl_input_set_mapping_keyboard(&context, SDLK_UP, BTN_TYPE_UP);
	sdl_input_set_mapping_keyboard(&context, SDLK_LEFT, BTN_TYPE_LEFT);
	sdl_input_set_mapping_keyboard(&context, SDLK_DOWN, BTN_TYPE_DOWN);
	sdl_input_set_mapping_keyboard(&context, SDLK_RIGHT, BTN_TYPE_RIGHT);

	sdl_input_set_mapping_keyboard(&context, SDLK_Q, BTN_TYPE_ACTION_0);
	sdl_input_set_mapping_keyboard(&context, SDLK_E, BTN_TYPE_ACTION_1);
	sdl_input_set_mapping_keyboard(&context, SDLK_SPACE, BTN_TYPE_SPACE);

	sdl_input_set_mapping_mouse(&context, 1, BTN_TYPE_UI_SELECT);
	sdl_input_set_mapping_mouse(&context, 3, BTN_TYPE_UI_EXTRA);

	GameState current_state = STATE_MENU;
	while (!quit)
	{
		quit = sdl_process_events(&context);

		SDL_SetRenderDrawColor(context.renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(context.renderer);

		itu_lib_imgui_frame_begin();

		switch (current_state)
		{
		case STATE_MENU:
			render_main_menu_ui(&context, &current_state, &survival_time, &quit);
			break;
		case STATE_PLAYING:
			gameplay_loop(&context, &current_state, &survival_time);
			break;
		case STATE_GAMEOVER:
			render_game_over_ui(&context, &current_state, &survival_time, &quit);
			break;
		default:
			break;
		}

#ifdef DEBUG
#ifdef ENABLE_DIAGNOSTICS
		{
			// ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4 { 33/255.0f, 33/255.0f, 33/255.0f, 255/255.0f });
			if (context.debug_ui_show)
			{
				if (ImGui::Begin("Debug UI", &context.debug_ui_show, ImGuiWindowFlags_NoCollapse))
				{
					ImGui::BeginTabBar("debug_ui_tab");
					if (ImGui::BeginTabItem("Context"))
					{
						// ImGui::Begin("itu_diagnostics");
						ImGui::Text("Timing");
						ImGui::LabelText("work", "%6.3f ms/f", (float)elapsed_work / (float)MILLIS(1));
						ImGui::LabelText("tot", "%6.3f ms/f", (float)elapsed_frame / (float)MILLIS(1));
						ImGui::LabelText("physics steps", "%d", context.physics_steps_count);

						ImGui::Separator();

						ImGui::Text("Player position");
						if(itu_entity_is_valid(id_player)) {
							Transform *transform_player = entity_get_data(id_player, Transform);
							ImGui::LabelText("x", "%f", transform_player->position.x);
							ImGui::LabelText("y", "%f", transform_player->position.y);
						}
						ImGui::Separator();

						ImGui::Text("Starting position");
						ImGui::LabelText("x", "%f", context.player_start_position.x);
						ImGui::LabelText("y", "%f", context.player_start_position.y);

						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Entities"))
					{
						itu_sys_estorage_debug_render(&context);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Resources"))
					{
						itu_sys_rstorage_debug_render(&context);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Extra"))
					{
						if (ImGui::Button("game_reset()"))
						{
							game_reset(&context);
						}
						if (ImGui::Button("draw map collisions"))
						{
							context.render_debug = !context.render_debug;
						}
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
				ImGui::End();
			}
		}
#endif
#endif
		// Destroy scheduled entites
		destroyEntitiesScheduled();

		itu_lib_imgui_frame_end(&context);

		SDL_GetCurrentTime(&walltime_work_end);
		elapsed_work = walltime_work_end - walltime_frame_beg;

		if (elapsed_work < TARGET_FRAMERATE_NS)
			SDL_DelayNS(TARGET_FRAMERATE_NS - elapsed_work);

		SDL_GetCurrentTime(&walltime_frame_end);
		elapsed_frame = walltime_frame_end - walltime_frame_beg;

		// render
		SDL_RenderPresent(context.renderer);

		context.delta = (float)elapsed_frame / (float)SECONDS(1);
		context.uptime += context.delta;
		context.elapsed_frame = elapsed_frame;
		walltime_frame_beg = walltime_frame_end;
	}

	SDL_DestroyRenderer(context.renderer);
    SDL_DestroyWindow(context.window);
    TTF_Quit();
    SDL_Quit();
}
