#include <config.hpp>
#include <itu_unity_include.hpp>
#include <utilities/room_parser.cpp>
#include <utilities/map_generator.cpp>

// ui colors
#define COLOR_BTN_DEFAULT color { 0.5f, 0.5f, 0.5f, 1.0f }
#define COLOR_BTN_HOVER   color { 0.75f, 0.75f, 0.75f, 1.0f }
#define COLOR_BTN_CLICK   color { 1.0f, 1.0f, 1.0f, 1.0f }

float design_speed_linear;
float design_speed_rotational;

enum Tags
{
	TAG_CAMERA_TARGET,
	TAG_ASTEROID
};

struct Tilemap
{
	SDL_Texture* texture;
	vec2f 		 pivot;
	int          num_rows;
	int          num_cols;
	int          tile_size; // in pixels
	int*		 tile_ids;  // array of arrays [num_rows][num_cols]

};

register_component(Tilemap)

struct PlayerData
{
	float curr_speed_linear;
	float curr_speed_rotational;

	ITU_EntityId target;
};
register_component(PlayerData)

struct EnemyData
{
	/* data */
	float curr_speed_linear;
};
register_component(EnemyData)

struct Health
{
	float max;
	float curr;
};
register_component(Health)

struct HealthRenderer
{
	float widget_base_w;
	ITU_EntityId target;
};
register_component(HealthRenderer)

struct TransformScreen
{
	vec2f position;
	vec2f scale;
	float rotation;
};
register_component(TransformScreen)

struct Sprite9Patch
{
	SDL_Texture* texture;
	SDL_FRect    rect;
	vec2f        pivot;
	vec2f        size;
	vec2f        margins_hor;
	vec2f        margins_ver;
	color        tint;
};
register_component(Sprite9Patch)

struct ImageButton
{
	TTF_Text* ttf_text; // owned

	void (*fn_callback_hover)(SDLContext* context, ITU_EntityId id);
	void (*fn_callback_click)(SDLContext* context, ITU_EntityId id);
};
register_component(ImageButton)

struct ShooterData
{
	Weapon weapon;
	unsigned int bullet_count;
	unsigned int next_bullet_idx;
	float cooldown_left;
	ITU_EntityId* bullets;
};
register_component(ShooterData)

static ITU_EntityId id_player;

static TTF_TextEngine* ttf_engine;

const int tile_mapping[] = {
	40, // wall 
	48 // floor 
};

const float PIXELS_PER_METER = (float)TEXTURE_PIXELS_PER_UNIT;
const float METERS_PER_PIXEL = 1.0f / PIXELS_PER_METER;

// ============================================================================================
// Map generation methods
// ============================================================================================

vec2f room_coordinate_to_world_position(int room_x, int room_y, int room_width, int room_height){
	float world_x = room_x * room_width;
	float world_y = room_y * room_height;
	return vec2f { world_x, world_y };
}

std::tuple<vec2f, vec2f> tile_coordinate_to_world_position(Tilemap* tilemap, Transform* transform, int tile_col, int tile_row, float tile_offset = -0.5f, vec2f pivot = vec2f{ 0.5f, 0.5f }){
	int tile_size = tilemap->tile_size;
	
	float width = transform->scale.x * (tile_size / (float)TEXTURE_PIXELS_PER_UNIT);
	float height = transform->scale.y * (tile_size / (float)TEXTURE_PIXELS_PER_UNIT);
	float x = transform->position.x + width * (tile_col + tile_offset) - (pivot.x * ROOM_NUM_TILES_X);
	float y = transform->position.y + height * (tile_row + tile_offset) - (pivot.y * ROOM_NUM_TILES_Y);

	return std::make_tuple(vec2f { width, height }, vec2f { x, y });
}

bool is_solid_tile(int tile_id){
	return tile_id == 0; 
}

// ============================================================================================
// Collision Callbacks 
// ============================================================================================

void system_collision_events(SDLContext* context, ITU_EntityId* entity_ids, int entity_ids_count)
{
	b2ContactEvents contactEvents = itu_sys_physics_get_contact_events();
	//iterate begin events
	for (int i = 0; i < contactEvents.beginCount; ++i) {

		b2ContactBeginTouchEvent* beginEvent = contactEvents.beginEvents + i;
		b2ShapeId shapeIdA = beginEvent->shapeIdA;
		b2ShapeId shapeIdB = beginEvent->shapeIdB;

		// shapeid to bodyid to entityid
		void* entityA_data = b2Body_GetUserData(b2Shape_GetBody(shapeIdA));
		void* entityB_data = b2Body_GetUserData(b2Shape_GetBody(shapeIdB));
		ITU_EntityId entityAId = value_cast(ITU_EntityId, entityA_data);
		ITU_EntityId entityBId = value_cast(ITU_EntityId, entityB_data);

		// Handle Collision Start Events
	
	}
	for (int i = 0; i < contactEvents.endCount; ++i) {
		b2ContactEndTouchEvent* event = contactEvents.endEvents + i;
		b2ShapeId shapeIdA = event->shapeIdA;
		b2ShapeId shapeIdB = event->shapeIdB;

		// shapeid to bodyid to entityid
		void* entityA_data = b2Body_GetUserData(b2Shape_GetBody(shapeIdA));
		void* entityB_data = b2Body_GetUserData(b2Shape_GetBody(shapeIdB));
		ITU_EntityId entityAId = value_cast(ITU_EntityId, entityA_data);
		ITU_EntityId entityBId = value_cast(ITU_EntityId, entityB_data);

		// Handle Collision End Events

	}

}


// ============================================================================================
// TMP methods
// ============================================================================================

void system_tilemap_render(SDLContext* context, ITU_EntityId* entity_ids, int entity_ids_count)
{
	for(int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		Tilemap* tilemap = entity_get_data(id, Tilemap);
		Transform* transform = entity_get_data(id, Transform);
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


void system_camera_target(SDLContext* context, ITU_EntityId* entity_ids, int entity_ids_count)
{
	for(int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		Transform* transform = entity_get_data(id, Transform);

		context->camera_active->world_position = transform->position;
	}
}

void lib_sprite_render_camera(SDLContext* context, Sprite* sprite, TransformScreen* transform)
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
		sprite->flip_horizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE
	);
}

void system_sprite_render_camera(SDLContext* context, ITU_EntityId* entity_ids, int entity_ids_count)
{
	for(int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		TransformScreen* transform = entity_get_data(id, TransformScreen);
		Sprite*              sprite    = entity_get_data(id, Sprite);

		lib_sprite_render_camera(context, sprite, transform);
	}

	// outline render target
	sdl_set_render_draw_color(context, { 1, 0, 1, 1 });
	SDL_RenderRect(context->renderer, NULL);
}


// ============================================================================================
// COMPONENT DEBUG UI RENDER methods
// ============================================================================================

void debug_ui_render_playerdata(SDLContext* context, void* data)
{
	PlayerData* data_player = (PlayerData*)data;

	ImGui::DragFloat("curr. linear speed", &data_player->curr_speed_linear);
	ImGui::DragFloat("curr. rotational speed", &data_player->curr_speed_rotational);

	itu_debug_ui_widget_entityid("target", data_player->target);
}

void debug_ui_render_health(SDLContext* context, void* data)
{
	Health* data_health = (Health*)data;

	ImGui::DragFloat("max", &data_health->max);
	ImGui::DragFloat("curr", &data_health->curr, 1, 0, data_health->max);
}

void debug_ui_render_healthrenderer(SDLContext* context, void* data)
{
	HealthRenderer* data_renderer = (HealthRenderer*)data;

	itu_debug_ui_widget_entityid("target", data_renderer->target);
	ImGui::DragFloat("base widget width", &data_renderer->widget_base_w);
}

void debug_ui_render_transformscreen(SDLContext* context, void* data)
{
	TransformScreen* data_transform = (TransformScreen*)data;

	ImGui::DragFloat2("position", &data_transform->position.x);
	ImGui::DragFloat2("scale", &data_transform->scale.x);

	float rotation_deg = data_transform->rotation * RAD_2_DEG;
	if(ImGui::DragFloat("rotation", &rotation_deg))
		data_transform->rotation = rotation_deg * DEG_2_RAD;

	itu_lib_render_draw_point(context->renderer, data_transform->position, 5, COLOR_YELLOW);
}

void debug_ui_render_sprite9patch(SDLContext* context, void* data)
{
	Sprite9Patch* data_sprite = (Sprite9Patch*)data;

	itu_sys_rstorage_debug_render_texture(data_sprite->texture, &data_sprite->texture, &data_sprite->rect);

	ImGui::DragFloat4("texture rect", &data_sprite->rect.x);
	ImGui::DragFloat2("pivot", &data_sprite->pivot.x);

	ImGui::DragFloat2("size", &data_sprite->size.x);
	ImGui::DragFloat2("margins ver.", &data_sprite->margins_hor.x);
	ImGui::DragFloat2("margins hor.", &data_sprite->margins_ver.x);

	ImGui::ColorEdit4("tint", &data_sprite->tint.r);
}


void debug_ui_render_imagebutton(SDLContext* context, void* data)
{
	ImageButton* data_imagebutton = (ImageButton*)data;
	//char* buf;
	//
	//TTF_SetTextString
	//ImGui::InputTextMultiline("text", buf, 1024);
	ImGui::LabelText("hover callback", "%p", data_imagebutton->fn_callback_hover);
	ImGui::LabelText("click callback", "%p", data_imagebutton->fn_callback_click);

	int wrap_width;
	TTF_GetTextWrapWidth(data_imagebutton->ttf_text, &wrap_width);

	int size[2];
	TTF_GetTextSize(data_imagebutton->ttf_text, &size[0], &size[1]);

	color c;
	TTF_GetTextColorFloat(data_imagebutton->ttf_text, &c.r, &c.g, &c.b, &c.a);

	TTF_Font* font = TTF_GetTextFont(data_imagebutton->ttf_text);
	TTF_Font* new_font;
	if(itu_sys_rstorage_debug_render_font(font, &new_font))
		TTF_SetTextFont(data_imagebutton->ttf_text, new_font);
	

	ImGui::InputInt2("size (readonly)", size);

	if(ImGui::DragInt("wrap width", &wrap_width))
		TTF_SetTextWrapWidth(data_imagebutton->ttf_text, wrap_width);

	if(ImGui::ColorEdit4("color", &c.r))
		TTF_SetTextColorFloat(data_imagebutton->ttf_text, c.r, c.g, c.b, c.a);
}

// ============================================================================================
//  
// ============================================================================================


void system_assign_player_target(SDLContext* context, ITU_EntityId* entity_ids, int entity_ids_count)
{
	if(!itu_entity_is_valid(id_player))
		return;

	PlayerData* player_data = entity_get_data(id_player, PlayerData);
	Transform* player_transform = entity_get_data(id_player, Transform);
	vec2f player_pos = player_transform->position;

	ITU_EntityId id_closest = ITU_ENTITY_ID_NULL;
	float closest_distance_sq = FLOAT_MAX_VAL;

	for(int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];

		Transform*   transform = entity_get_data(id, Transform);

		float curr_distance_sq = distance_sq(player_pos, transform->position);

		if(curr_distance_sq < closest_distance_sq)
		{
			id_closest = id;
			closest_distance_sq = curr_distance_sq;
		}
	}

	player_data->target = id_closest;
}

void system_player_update(SDLContext* context, ITU_EntityId* entity_ids, int entity_ids_count)
{
	for(int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		Transform*      transform    = entity_get_data(id, Transform);
		PlayerData* data         = entity_get_data(id, PlayerData);
		PhysicsData*    physics_data = entity_get_data(id, PhysicsData);

		vec2f dir = VEC2F_ZERO;
		if(context->btn_isdown[BTN_TYPE_UP])
			dir.y += 1;
		if(context->btn_isdown[BTN_TYPE_DOWN])
			dir.y -= 1;
		if(context->btn_isdown[BTN_TYPE_LEFT])
			dir.x -= 1;
		if(context->btn_isdown[BTN_TYPE_RIGHT])
			dir.x += 1;

		physics_data->velocity = normalize(dir) * 5;

		float target_rotation = 0.0f;
		if(itu_entity_is_valid(data->target))
		{
			Transform* target_transform = entity_get_data(data->target, Transform);
			vec2f lookat = normalize(target_transform->position - transform->position);
			target_rotation = SDL_atan2f(lookat.y, lookat.x) - PI_HALF;
		}
		// asymptotic approach
		transform->rotation = lerp(transform->rotation, target_rotation, 0.15f);
	}
}

void system_health(SDLContext* context, ITU_EntityId* entity_ids, int entity_ids_count)
{
	for(int i = 0; i < entity_ids_count; ++i)
	{
		ITU_EntityId id = entity_ids[i];
		HealthRenderer* renderer = entity_get_data(id, HealthRenderer);

		if(!itu_entity_is_valid(renderer->target))
			continue;

		Sprite9Patch* sprite = entity_get_data(id, Sprite9Patch);
		Health* health = entity_get_data(renderer->target, Health);

		if(context->btn_isjustpressed[BTN_TYPE_SPACE])
			health->curr = SDL_clamp(health->curr - health->max / 10, 0, 100);

		sprite->size.x = renderer->widget_base_w * (health->curr / health->max);
	}
}

void system_player_shooting(SDLContext* context, ITU_EntityId* entity_ids, int entity_ids_count){
	for (int i = 0; i < entity_ids_count; i++)
	{
		ITU_EntityId id = entity_ids[i];
		ShooterData* shooter = entity_get_data(id,ShooterData);
		
		if(shooter->cooldown_left <= shooter->weapon.cooldown){
			shooter->cooldown_left += context->delta;
		}
		
		if(context->btn_isjustpressed_space){
			Transform* transform = entity_get_data(id,Transform);
			PlayerData* pd = entity_get_data(id,PlayerData);
	
			ITU_EntityId null_ent = ITU_ENTITY_ID_NULL;
			Transform* target;
			vec2f direction;
			if(pd->target != null_ent){
	
				target = entity_get_data(pd->target,Transform);
				direction = normalize(target->position - transform->position);
			}
			else{
				direction = VEC2F_RIGHT;
			}
	
	
			int bullet_amount = shooter->weapon.bullets_per_shot;
			unsigned int start_index = shooter->next_bullet_idx;
			for (int j = 0; j < bullet_amount; j++)
			{
				ITU_EntityId bullet_id = shooter->bullets[start_index];
				BulletData* bd = entity_get_data(bullet_id,BulletData);
				Transform* bt = entity_get_data(bullet_id,Transform);
	
				bd->is_active = true;
				bd->direction = direction;
				bd->speed = shooter->weapon.bullet_speed;
				bd->update_behaviour = shooter->weapon.fn_bullet_behaviour;
	
				bt->position = transform->position;
				
				start_index++;
				if(start_index >= shooter->bullet_count){
					start_index = 0;
				}
			}
			shooter->next_bullet_idx = start_index;
		}

	}
	
}

void system_bullet_update(SDLContext* context, ITU_EntityId* entity_ids, int entity_ids_count){
	for (int i = 0; i < entity_ids_count; i++)
	{
		ITU_EntityId id = entity_ids[i];
		BulletData* bd = entity_get_data(id,BulletData);
		
		if(!bd->is_active) continue;

		PhysicsData* pd = entity_get_data(id,PhysicsData);
		
		bd->update_behaviour(pd,bd->direction,bd->speed);
	}
}


bool is_same_room(Point r1, Point r2){
	return r1.x == r2.x && r1.y == r2.y;
}

static void game_init(SDLContext* context)
{

	itu_sys_rstorage_texture_load(context, "data/kenney/tiny_dungeon_packed.png", SDL_SCALEMODE_NEAREST);
	itu_sys_rstorage_font_load(context, "data/ARIALBD.TTF", 42);

	itu_sys_estorage_init(512);
	itu_sys_physics_init(context);

	enable_component(Tilemap);
	enable_component(PlayerData);
	enable_component(TransformScreen);
	
	add_component_debug_ui_render(PlayerData, debug_ui_render_playerdata);
	add_component_debug_ui_render(TransformScreen, debug_ui_render_transformscreen);
	//TODO: add_component_debug_ui_render(Tilemap, debug_ui_render_tilemap);

	itu_sys_estorage_tag_set_debug_name(TAG_CAMERA_TARGET, "camera target");
	itu_sys_estorage_tag_set_debug_name(TAG_ASTEROID, "asteroid");
	
	add_system(system_tilemap_render				, component_mask(Transform) | component_mask(Tilemap)          , 0, true);
	add_system(system_assign_player_target      , component_mask(Transform), tag_mask(TAG_ASTEROID), false);
	add_system(system_player_update             , component_mask(Transform) | component_mask(PhysicsData) | component_mask(PlayerData)  , 0, false);
	add_system(system_sprite_render_camera      , component_mask(TransformScreen) | component_mask(Sprite)          , 0, true);
	add_system(system_camera_target             , component_mask(Transform), tag_mask(TAG_CAMERA_TARGET), false);

	add_system(system_player_shooting, component_mask(PlayerData) | component_mask(ShooterData) | component_mask(Transform),0,false);
	add_system(system_bullet_update, component_mask(BulletData) | component_mask(PhysicsData) | component_mask(Transform), 0, false);
	add_system(system_collision_events			, component_mask(Transform), 0, false );
	add_system(itu_system_sprite_render , component_mask(Transform)   | component_mask(Sprite)         , 0, true);
	

}

static void game_reset(SDLContext* context)
{
	// TMP get textures pointers
	SDL_Texture* texture_tiles = itu_sys_rstorage_texture_get_ptr(0);
	TTF_Font* font_bold = itu_sys_rstorage_font_get_ptr(0);

	itu_sys_estorage_clear_all_entities();

	b2WorldDef world_def = b2DefaultWorldDef();
	world_def.gravity.y = 0; //Since top-down there is no gravity (anything falling downwards)
	itu_sys_physics_reset(&world_def);

	SDL_assert(ENTITY_COUNT <= ENTITIES_COUNT_MAX);

	std::vector<vec2f> enemy_placements;
	//tilemap
	{
		Map map = generate_map(10, 10, 10, context->prng);
		Room room = generate_room_matrix_from_file("../exam_project/room_templates/simple_room.txt");
		
		Point start_room = map.room_locations.at(0);
		context->player_start_position = room_coordinate_to_world_position(start_room.x, start_room.y, ROOM_NUM_TILES_X, ROOM_NUM_TILES_Y);
		
		for (Point room_loc : map.room_locations){
			ITU_EntityId id_tilemap = itu_entity_create();
			itu_entity_set_debug_name(id_tilemap, "tilemap");
			
			Tilemap tilemap; 
			
			tilemap.num_cols = room.num_cols;
			tilemap.num_rows = room.num_rows;
			tilemap.tile_ids = room.tiles;
			tilemap.texture = texture_tiles;
			tilemap.tile_size = 16;
			tilemap.pivot = { 0.5f, 0.5f };

			Transform transform = TRANSFORM_DEFAULT;
			transform.scale = vec2f { 1.0f, 1.0f };
			transform.position  = room_coordinate_to_world_position(room_loc.x, room_loc.y, ROOM_NUM_TILES_X, ROOM_NUM_TILES_Y);
			
			entity_add_component(id_tilemap, Transform, transform);
			entity_add_component(id_tilemap, Tilemap , tilemap);
			int enemies_placed = 0;
			// player vs tile collision detection
			for (int r = 0; r < room.num_rows; ++r) {

				for(int c = 0; c < room.num_cols; ++c) {
					int idx = r * room.num_cols + c;
					int tile_id = tilemap.tile_ids[idx];

					vec2f tile_position;
					std::tie(std::ignore, tile_position) = tile_coordinate_to_world_position(&tilemap, &transform, c, r, 0);
					if(!is_solid_tile(tile_id)){
						if(enemies_placed >= MAX_ENEMIES_PER_ROOM || is_same_room(room_loc,start_room)){
							continue;
						}
						uint32_t place = random_up_to(100,context->prng);
						if(place < 2){
							enemies_placed++;
							enemy_placements.push_back(tile_position);
						}
					} else {
						int width = 1;
						while (c + width < room.num_cols && is_solid_tile(tilemap.tile_ids[idx + width])) ++width;
				
						ITU_EntityId row_id = itu_entity_create();
						itu_entity_set_debug_name(row_id, "tile-collider");

						b2BodyDef tile_body_def = b2DefaultBodyDef();
						tile_body_def.userData = value_cast(void*, row_id);
						tile_body_def.type = b2_staticBody;

						//Calculate center of rectangle
						float row_end_x = tile_position.x + width;
						tile_body_def.position = b2Vec2 { (tile_position.x + row_end_x) * 0.5f - 0.5f, tile_position.y};

						b2BodyId body_id = itu_sys_physics_add_body(value_cast(void*, row_id), &tile_body_def);
						b2ShapeDef shape_def = b2DefaultShapeDef();
						shape_def.enableContactEvents = false;
						
						b2Polygon box = b2MakeBox(width * 0.5f, 0.5f);
						
						ShapeData shape_data = { 0 };
						shape_data.shape_id = b2CreatePolygonShape(body_id, &shape_def, &box);
						entity_add_component(row_id, ShapeData, shape_data);

						//Skip consumed
						c += (width - 1);
					}

							
				}
				
			}
		}
			
	}
	
	
	// Enemies
	{
		for(vec2f pos : enemy_placements){
			ITU_EntityId enemy_id = itu_entity_create();
			itu_entity_set_debug_name(enemy_id, "enemy");
			Transform transform = TRANSFORM_DEFAULT;
			transform.position = pos;

			Sprite sprite;
			itu_lib_sprite_init(&sprite,texture_tiles,itu_lib_sprite_get_rect(0,9,TEXTURE_PIXELS_PER_UNIT, TEXTURE_PIXELS_PER_UNIT));

			PhysicsData physics_data = { 0 };
			physics_data.ignore_rotation = true;

			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.userData = value_cast(void*, enemy_id);
			body_def.type = b2_dynamicBody;
			b2Circle circle = { 0 };
			circle.radius = 0.25f;	
			body_def.position = value_cast(b2Vec2,transform.position);
			physics_data.body_id = itu_sys_physics_add_body(value_cast(void*, enemy_id), &body_def);

			b2ShapeDef shape_def = b2DefaultShapeDef();
			shape_def.enableContactEvents = true;

			ShapeData shape_data;
			shape_data.shape_id = b2CreateCircleShape(physics_data.body_id, &shape_def, &circle);

			EnemyData ed = { 0 };

			entity_add_component(enemy_id, Transform, transform);
			entity_add_component(enemy_id, PhysicsData, physics_data);
			entity_add_component(enemy_id, ShapeData, shape_data);
			entity_add_component(enemy_id, EnemyData, ed);
			entity_add_component(enemy_id, Sprite, sprite);
		}	
	}

	// player
	{
		id_player = itu_entity_create();
		itu_entity_set_debug_name(id_player, "player");
		Transform transform = TRANSFORM_DEFAULT;
		transform.position = context->player_start_position;

		Sprite sprite;
		itu_lib_sprite_init(&sprite, texture_tiles, itu_lib_sprite_get_rect(0, 8, TEXTURE_PIXELS_PER_UNIT, TEXTURE_PIXELS_PER_UNIT));

		PlayerData data = { 0 };

		PhysicsData physics_data = { 0 };
		physics_data.ignore_rotation = true;
		
		b2BodyDef body_def = b2DefaultBodyDef();
		body_def.userData = value_cast(void*, id_player);
		body_def.type = b2_dynamicBody;
		body_def.position = value_cast(b2Vec2, transform.position);
		physics_data.body_id = itu_sys_physics_add_body(value_cast(void*, id_player), &body_def);
		
		b2ShapeDef shape_def = b2DefaultShapeDef();
		shape_def.enableContactEvents = true;
		
		b2Circle circle = { 0 };
		circle.radius = 0.25f;
		shape_def.isSensor = false;

		ShapeData shape_data;
		shape_data.shape_id = b2CreateCircleShape(physics_data.body_id, &shape_def, &circle);

		entity_add_component(id_player, Transform     , transform);
		entity_add_component(id_player, Sprite        , sprite);
		entity_add_component(id_player, PlayerData, data);
		entity_add_component(id_player, PhysicsData   , physics_data);
		entity_add_component(id_player, ShapeData     , shape_data);
		itu_entity_tag_add(id_player, TAG_CAMERA_TARGET);
	}

	//bullets
	{
		b2Capsule capsule;
		capsule.radius = 0.15f;

		ShooterData shooter = { 0 };
		shooter.bullet_count = BULLET_POOL_SIZE;
		shooter.weapon = basic_weapon;

		for (size_t i = 0; i < BULLET_POOL_SIZE; i++)
		{
			ITU_EntityId id = itu_entity_create();
			shooter.bullets[i] = id;

			itu_entity_set_debug_name(id,"bullet" + i);
			Transform transform = TRANSFORM_DEFAULT;

			Sprite sprite;
			itu_lib_sprite_init(&sprite,texture_tiles,itu_lib_sprite_get_rect(11,10,TEXTURE_PIXELS_PER_UNIT,TEXTURE_PIXELS_PER_UNIT));

			BulletData bd = { 0 };
			
			PhysicsData pd = { 0 };
			pd.ignore_rotation = true;
			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.position = value_cast(b2Vec2, transform.position);
			body_def.userData = value_cast(void*, id);
			pd.body_id = itu_sys_physics_add_body(value_cast(void*, id), &body_def);

			ShapeData shape_data;
			b2ShapeDef shape_def = b2DefaultShapeDef();
			shape_def.enableContactEvents = true;
			shape_data.shape_id = b2CreateCapsuleShape(pd.body_id,&shape_def,&capsule);
			entity_add_component(id, Transform, transform);
			entity_add_component(id, Sprite, sprite);
			entity_add_component(id, BulletData, bd);
			entity_add_component(id, PhysicsData, pd);
			entity_add_component(id, ShapeData, shape_data);
		}
		entity_add_component(id_player,ShooterData,shooter);
	}

}

int main(void)
{
	bool quit = false;
	SDLContext context = { 0 };
	
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

	// set debug UI shown by default (new and shiny, let's showcase it)
	context.debug_ui_show = true;

	game_init(&context);
	game_reset(&context);

	SDL_Time walltime_frame_beg;
	SDL_Time walltime_frame_end;
	SDL_Time walltime_work_end;
	SDL_Time elapsed_work = 0;
	SDL_Time elapsed_frame = 0;

	SDL_GetCurrentTime(&walltime_frame_beg);
	walltime_frame_end = walltime_frame_beg;

	sdl_input_set_mapping_keyboard(&context, SDLK_W,     BTN_TYPE_UP);
	sdl_input_set_mapping_keyboard(&context, SDLK_A,     BTN_TYPE_LEFT);
	sdl_input_set_mapping_keyboard(&context, SDLK_S,     BTN_TYPE_DOWN);
	sdl_input_set_mapping_keyboard(&context, SDLK_D,     BTN_TYPE_RIGHT);
	sdl_input_set_mapping_keyboard(&context, SDLK_Q,     BTN_TYPE_ACTION_0);
	sdl_input_set_mapping_keyboard(&context, SDLK_E,     BTN_TYPE_ACTION_1);
	sdl_input_set_mapping_keyboard(&context, SDLK_SPACE, BTN_TYPE_SPACE);

	sdl_input_set_mapping_mouse(&context, 1, BTN_TYPE_UI_SELECT);
	sdl_input_set_mapping_mouse(&context, 3, BTN_TYPE_UI_EXTRA);

	while(!quit)
	{
		quit = sdl_process_events(&context);

		SDL_SetRenderDrawColor(context.renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(context.renderer);
		
		itu_lib_imgui_frame_begin();

		// update
		itu_sys_estorage_systems_update(&context);

		// render
		itu_sys_estorage_render_update(&context);
		if (context.render_debug) itu_sys_physics_debug_draw();
		

#ifdef ENABLE_DIAGNOSTICS
		{
			//ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4 { 33/255.0f, 33/255.0f, 33/255.0f, 255/255.0f });
			if(context.debug_ui_show)
			{
				if(ImGui::Begin("Debug UI", &context.debug_ui_show, ImGuiWindowFlags_NoCollapse))
				{
					ImGui::BeginTabBar("debug_ui_tab");
					if(ImGui::BeginTabItem("Context"))
					{
						//ImGui::Begin("itu_diagnostics");
						ImGui::Text("Timing");
						ImGui::LabelText("work", "%6.3f ms/f", (float)elapsed_work  / (float)MILLIS(1));
						ImGui::LabelText("tot",  "%6.3f ms/f", (float)elapsed_frame / (float)MILLIS(1));
						ImGui::LabelText("physics steps",  "%d", context.physics_steps_count);
						
						ImGui::Separator();

						ImGui::Text("Player position");
						Transform* transform_player = entity_get_data(id_player, Transform);
						ImGui::LabelText("x", "%f", transform_player->position.x);
						ImGui::LabelText("y", "%f", transform_player->position.y);

						ImGui::Separator();

						ImGui::Text("Starting position");
						ImGui::LabelText("x", "%f", context.player_start_position.x);
						ImGui::LabelText("y", "%f", context.player_start_position.y);

						ImGui::EndTabItem();
					}
					if(ImGui::BeginTabItem("Entities"))
					{
						itu_sys_estorage_debug_render(&context);
						ImGui::EndTabItem();
					}
					if(ImGui::BeginTabItem("Resources"))
					{
						itu_sys_rstorage_debug_render(&context);
						ImGui::EndTabItem();
					}
					if(ImGui::BeginTabItem("Extra"))
					{
						if(ImGui::Button("game_reset()")){
							game_reset(&context);
						}
						if(ImGui::Button("draw map collisions"))
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

		itu_lib_imgui_frame_end(&context);

		SDL_GetCurrentTime(&walltime_work_end);
		elapsed_work = walltime_work_end - walltime_frame_beg;

		if(elapsed_work < TARGET_FRAMERATE_NS)
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
}
