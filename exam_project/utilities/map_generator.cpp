#ifndef ITU_UNITY_BUILD
#include <itu_common.hpp>
#endif

struct Point
{
    int x;
    int y;
};

enum Direction
{
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
};

struct Map
{
    std::vector<Point> room_locations;
};

bool check_bounds(int x, int y, int width, int height){
    return (x >= 0 && x < width && y >= 0 && y < height);
}

bool is_direction_allowed(int x, int y, int width, int height, Direction dir){
    switch (dir)
    {
    case DIR_UP:
        return check_bounds(x, y + 1, width, height);
    case DIR_DOWN:
        return check_bounds(x, y - 1, width, height);
    case DIR_LEFT:
        return check_bounds(x - 1, y, width, height);
    case DIR_RIGHT:
        return check_bounds(x + 1, y, width, height);
    default:
        return false;
    }
}

bool room_exists_at(const Map& map, int x, int y){
    for (const auto& room : map.room_locations){
        if (room.x == x && room.y == y){
            return true;
        }
    }
    return false;
}

Direction choose_new_direction(Direction current_direction, PRNG* engine){
    Direction new_direction;
    do{
        new_direction = Direction(random_up_to(4, engine));
    } while (new_direction == current_direction);
    
    return new_direction;
}


Map generate_map(int width, int height, int num_rooms, PRNG* engine){
    uint32_t x = random_up_to(width, engine);
    uint32_t y = random_up_to(height, engine);

    Direction current_direction = Direction(random_up_to(4, engine));
    
    int direction_chance = 10; // initial chance to change direction

    
    Map map;
    map.room_locations.push_back({static_cast<int>(x), static_cast<int>(y)});

    while (map.room_locations.size() < num_rooms){
        if (is_direction_allowed(x, y, width, height, current_direction)){
            switch (current_direction)
            {
                case DIR_UP:
                    y += 1;
                    break;
                case DIR_DOWN:
                    y -= 1;
                    break;
                case DIR_LEFT:
                    x -= 1;
                    break;
                case DIR_RIGHT:
                    x += 1;
                    break;
                default:
                    break;
            }
            if (!room_exists_at(map, x, y)){
                map.room_locations.push_back({static_cast<int>(x), static_cast<int>(y)});
            }

            int roll = random_up_to(100, engine);
            if (roll <= direction_chance){
                current_direction = choose_new_direction(current_direction, engine);
                direction_chance = 10; // reset chance
            } else {
                direction_chance += 20; // increase chance to change direction next time
            }        

        } else {
            current_direction = choose_new_direction(current_direction, engine);
            direction_chance = 10; // reset chance
        }
    }
    return map;
}
