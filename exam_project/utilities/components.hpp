#ifndef ITU_UNITY_BUILD
#include <itu_common.hpp>
#include <itu_entity_storage.hpp>
#include <weapon_utils.hpp>
#endif

struct Tilemap
{
	SDL_Texture* texture;
	vec2f 		 pivot;
	int          num_rows;
	int          num_cols;
	int          tile_size; // in pixels
	int* tile_ids;  // [num_rows][num_cols]
};
register_component(Tilemap)

struct PlayerData
{
	float curr_speed_linear;
	float curr_speed_rotational;
	vec2f rotation;

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
	float elapsed;
	float grace_period;
};
register_component(Health)

struct HealthRenderer
{
	float widget_base_w;
	ITU_EntityId target;
};
register_component(HealthRenderer)

struct CooldownRenderer
{
	float widget_base_w;
	ITU_EntityId target;
};
register_component(CooldownRenderer)

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
	ITU_EntityId bullets[BULLET_POOL_SIZE];
};
register_component(ShooterData)