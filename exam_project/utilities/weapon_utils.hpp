#ifndef ITU_UNITY_BUILD
#include <itu_common.hpp>
#include <itu_entity_storage.hpp>
#include <itu_sys_physics.hpp>
#endif

typedef void (*BulletBehaviourUpdateFunction) (PhysicsData*pd, vec2f dir, float speed);

struct BulletData
{
	bool is_active;
	uint damage;
	float speed;
	vec2f direction;
	BulletBehaviourUpdateFunction update_behaviour;
};
register_component(BulletData)


enum ShotPattern{
	STRAIGHT_AHEAD, SPREAD,
};

struct Weapon
{
	unsigned int bullets_per_shot;
	unsigned int damage;
	float cooldown;
	float bullet_speed;
	BulletBehaviourUpdateFunction fn_bullet_behaviour;
	ShotPattern pattern;
};

void fn_straigh_shot(PhysicsData* pd, vec2f dir, float speed){
	pd->velocity = dir * speed;
}

Weapon basic_weapon = { 1, 1, 1, 1, fn_straigh_shot, STRAIGHT_AHEAD};