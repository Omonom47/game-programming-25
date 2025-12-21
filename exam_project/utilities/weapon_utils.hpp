#ifndef ITU_UNITY_BUILD
#include <itu_common.hpp>
#include <itu_entity_storage.hpp>
#include <itu_sys_physics.hpp>
#endif

typedef void (*BulletBehaviourUpdateFunction) (PhysicsData*pd, vec2f dir, float speed);

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

void fn_straight_shot(PhysicsData* pd, vec2f dir, float speed){
	pd->velocity = dir * speed;
}

//Weapons for testing
Weapon basic_weapon = { 1, 1, 1, 6, fn_straight_shot, STRAIGHT_AHEAD };
Weapon basic_spread = { 5, 1, 1, 6, fn_straight_shot, SPREAD };

unsigned int damage_max_fn(int bullet_amount){
	int max = 255;
	max -= 106/bullet_amount;
	max /= bullet_amount;
	max -= 63/bullet_amount;

	return max;
}

unsigned int damage_min_fn(int bullet_amount){
	return -3*bullet_amount + 25;
}

float cooldown_floor_fn(int damage){
	float min = 0.3f;
	min += (damage/10)*0.12f;
	return min;
}

Weapon generate_weapon(PRNG* engine){
	Weapon ret;

	uint32_t bullets = random_in_range(1,8,engine);
	ret.bullets_per_shot = bullets;

	uint32_t damage = random_in_range(damage_min_fn(bullets)
		,damage_max_fn(bullets),engine);
	ret.damage = damage;

	float min_cooldown = cooldown_floor_fn(damage*bullets);
	float cooldown = random_float_in_range(min_cooldown,2.6f,engine);
	ret.cooldown = cooldown;

	float speed = random_float_in_range(6,10,engine);
	ret.bullet_speed = speed;

	ret.fn_bullet_behaviour = fn_straight_shot;
	
	if (is_odd(bullets) && is_odd(random_up_to(10,engine)))
	{
		ret.pattern = SPREAD;
	}else
		ret.pattern = STRAIGHT_AHEAD;

#ifdef DEBUG
	printf("Weapon:\n bullets: %d\n damage: %d\n cooldown: %f\n speed: %f\n", bullets,damage,cooldown, speed);
#endif
	return ret;
}