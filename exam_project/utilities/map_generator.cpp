#ifndef ITU_UNITY_BUILD
#include <config.hpp>
#include <itu_common.hpp>
#include <components.hpp>
#endif

struct Point{
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
    int room_rows;
    int room_cols;
};

const int WALL = 0;
const int EMPTY = 1;
const int ALTERNATE_FLOOR = 2;

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

std::vector<Direction> has_neighbor_directions(Map map, int x, int y) {
    std::vector<Direction> directions;
    if (room_exists_at(map, x, y + 1)) directions.push_back(DIR_UP);
    if (room_exists_at(map, x, y - 1)) directions.push_back(DIR_DOWN);
    if (room_exists_at(map, x - 1, y)) directions.push_back(DIR_LEFT);
    if (room_exists_at(map, x + 1, y)) directions.push_back(DIR_RIGHT);
    return directions;
}

Map generate_layout(int width, int height, int num_rooms, PRNG* engine){
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


bool is_out_of_bounds(int x, int y, int rows, int cols){
    return x < 0 || x >= cols || y < 0 || y >= rows; 
}

int live_neighbors(int* grid, int x, int y, int rows, int cols){
    int count = 0;

     // Check all 8 neightbors
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue; // Skip self

            int neighbor_x = x + j;
            int neighbor_y = y + i;
            
            // Concider out of bound cells as wall
            if (is_out_of_bounds(neighbor_x, neighbor_y, rows, cols)) {
                count++; 
            } // Otherwise check if neighbor is wall
            else if (grid[neighbor_y * cols + neighbor_x] == WALL) {
            count++;
            }
        }
    }
    return count;
}

void update_grid(int* grid, int wall_threshold,int rows,int cols){

    std::vector<int> updated(rows*cols); // Changed due to compiler complaining

    const int SURVIVAL_THRESHOLD = 4; // Wall stays wall if it has 4 more wall neighbors
    const int BIRTH_THRESHOLD = 5; // Empty becomes wall if it has 5 or more wall neighbors


    for (int i = 0; i < rows; i++){
        for (int j = 0; j < cols; j++){

            int live = live_neighbors(grid, j, i, rows, cols);
            int idx = i*cols+j;
            int cell = grid[idx];

            if (cell == WALL){
                if (live >= SURVIVAL_THRESHOLD)
                    updated[idx] = WALL;
                else
                    updated[idx] = EMPTY;
            } else {
                if (live >= BIRTH_THRESHOLD)
                    updated[idx] = WALL;
                else
                    updated[idx] = EMPTY;
            }
        } 
    }

    // Apply updates to the original grid
    for (int i = 0; i < rows * cols; i++) {
        grid[i] = updated[i];
    }
}

void generate_map_cellular_automata(int* grid, int iterations, int wall_threshold, int rows, int cols, PRNG* engine){

    // Setup random grid
    for (int i = 0; i < rows; i++){
        for (int j = 0; j < cols; j++)
        {
            if (random_up_to(100, engine) < wall_threshold)
                grid[i*cols+j] = WALL;
            else
                grid[i*cols+j] = EMPTY;
        }
    }

    for (int i = 0; i < iterations; i++)
    {
        update_grid(grid, wall_threshold,rows,cols);
    }

    // TODO: post processing of the generated grid
    
}

void enclose_room(int* grid, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // If on the border, set to WALL
            if (i == 0 || i == rows - 1 || j == 0 || j == cols - 1) {
                grid[i * cols + j] = WALL;
            }
        }
    }
}


void keep_largest_open_area(int* grid, int rows, int cols) {
    std::vector<bool> visited(rows * cols, false);
    std::vector<Point> largest_area;
    size_t largest_area_size = 0;

    std::vector<Point> directions = {
        {0, 1},  // Up
        {0, -1}, // Down
        {-1, 0}, // Left
        {1, 0}   // Right
    };

    // Implement BFS to find the connected open areas
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int idx = i * cols + j;
            if (grid[idx] != EMPTY || visited[idx]) {
                continue;
            }
            // Start BFS
            std::queue<Point> q;
            std::vector<Point> current_area;
            q.push({j, i});

            while (!q.empty()) {
                Point p = q.front();
                q.pop();

                int point_idx = p.y * cols + p.x;
                if (visited[point_idx]) {
                    continue;
                }
                visited[point_idx] = true;
                
                // Traverse neighbors
                for (const auto& dir : directions) {
                    int neighbor_x = p.x + dir.x;
                    int neighbor_y = p.y + dir.y;
                    int neighbor_idx = neighbor_y * cols + neighbor_x;

                    if (neighbor_x >= 0 && neighbor_x < cols &&
                        neighbor_y >= 0 && neighbor_y < rows &&
                        grid[neighbor_idx] == EMPTY &&
                        !visited[neighbor_idx]) {
                        q.push({neighbor_x, neighbor_y});
                        current_area.push_back({neighbor_x,neighbor_y});
                    }
                }
            }
            // Check if current area is the largest
            if (current_area.size() > largest_area_size) {
                largest_area_size = current_area.size();
                largest_area = current_area;
            }

        }
    }

    // Clear the grid and keep only the largest area
    for (int i = 0; i < rows * cols; i++) {
        grid[i] = WALL;
    }
    for (const auto& p : largest_area) {
        int idx = p.y * cols + p.x;
        grid[idx] = EMPTY;
    }
}

int get_room_index(Map map, Point room) {
    for (int i = 0; i < map.room_locations.size(); i++) {
        if (map.room_locations[i].x == room.x && map.room_locations[i].y == room.y) {
            return i;
        }
    }
    return -1; // Not found
}

int get_distance_from_edge_to_open_area(int* grid, int rows, int cols, Point point, Point dir) {
    int x = point.x;
    int y = point.y;
    int distance = 0;

    int max_steps = (dir.x != 0) ? cols : rows;
    for (int i = 0; i < max_steps; i++) {
        // Boundary check
        if (!check_bounds(x, y, cols, rows)) {
            return 9999; // Penalty for going out of bounds
        }
        if (grid[y * cols + x] == EMPTY) {
            return distance;
        }
        x += dir.x;
        y += dir.y;
        distance++;
    }
    return 9999; // Penalty, as we didn't find an empty cell
}

void carve_path(int* grid, int rows, int cols, Point start, int length, Point dir, int padding) {
    int x = start.x;
    int y = start.y;

    for(int i = 0; i <= length; i++) {
        
        for (int p = -padding;p <= padding; p++) {
            int target_x = x;
            int target_y = y;
            if (dir.x != 0) {
                target_y += p;
            } else if (dir.y != 0) {
                target_x += p;
            }

            if(check_bounds(target_x, target_y, cols, rows)) {
                int idx = target_y * cols + target_x;
                grid[idx] = EMPTY;
            }
             
        }
        x += dir.x;
        y += dir.y;
    }
}


void create_path_between_rooms(Map map, Tilemap* tilemaps, Point room_a, Point room_b) {
    int* grid_a = tilemaps[get_room_index(map, room_a)].tile_ids;
    int* grid_b = tilemaps[get_room_index(map, room_b)].tile_ids;

    int rows = map.room_rows;
    int cols = map.room_cols;

    // Calculate direction from room_a to room_b
    Point direction = { 0, 0};
    direction.x = (room_b.x > room_a.x) - (room_b.x < room_a.x);
    direction.y = (room_b.y > room_a.y) - (room_b.y < room_a.y);

    // Determine wall orientation
    bool is_horizontal = (direction.x != 0);
    int wall_length = is_horizontal ? rows : cols;
    int room_depth = is_horizontal ? cols : rows;

    // Determinte wall coordinates
    bool is_positive_direction = (is_horizontal && direction.x > 0) || (!is_horizontal && direction.y > 0);
    int wall_x = is_positive_direction ? room_depth - 1 : 0;
    int wall_y = is_positive_direction ? 0 : room_depth - 1;

    // Scan directions
    Point scan_room_a_direction = { -direction.x, -direction.y };
    Point scan_room_b_direction = direction;

    // Find best wall position to create a path
    int best_wall_pos = -1;
    int best_distance = 9999;
    
    int padding = 1; // Carve a wider path
    // Skip corners when scanning
    for (int i = padding; i < wall_length - padding; ++i) {
        Point point_a = is_horizontal ? Point{ wall_x, i } : Point{ i, wall_x };
        Point point_b = is_horizontal ? Point{ wall_y, i } : Point{ i, wall_y };

        int distance_a = get_distance_from_edge_to_open_area(grid_a, rows, cols, point_a, scan_room_a_direction);
        int distance_b = get_distance_from_edge_to_open_area(grid_b, rows, cols, point_b, scan_room_b_direction);

        if (distance_a + distance_b < best_distance) {
            best_distance = distance_a + distance_b;
            best_wall_pos = i;

            // Early exit if we found a good enough path
            if (best_distance <= 5) {
                break;
            }
        }
    }

    // Carve path in both rooms at the best wall position
    if (best_wall_pos != -1) {
        Point path_point_a = is_horizontal ? Point{ wall_x, best_wall_pos } : Point{ best_wall_pos, wall_x };
        Point path_point_b = is_horizontal ? Point{ wall_y, best_wall_pos } : Point{ best_wall_pos, wall_y };

        int length_a = get_distance_from_edge_to_open_area(grid_a, rows, cols, path_point_a, scan_room_a_direction);
        int length_b = get_distance_from_edge_to_open_area(grid_b, rows, cols, path_point_b, scan_room_b_direction);

        carve_path(grid_a, rows, cols, path_point_a, length_a, scan_room_a_direction, padding);
        carve_path(grid_b, rows, cols, path_point_b, length_b, scan_room_b_direction, padding);

    }
}


Map generate_map(Tilemap* tilemaps, int width, int height, int num_rooms, PRNG* engine) {
    Map map_layout = generate_layout(width, height, num_rooms, engine);

    const int rows = ROOM_NUM_TILES_Y; 
    const int cols = ROOM_NUM_TILES_X;
    size_t size = rows * cols;

    map_layout.room_cols = cols;
    map_layout.room_rows = rows;

    // Create tilemaps for each room
    for (int i = 0; i < num_rooms; i++) {

        Tilemap tilemap; 
        
        tilemap.num_cols = cols;
        tilemap.num_rows = rows;
        tilemap.tile_ids = new int[size];
        generate_map_cellular_automata(tilemap.tile_ids, 12, 45, rows, cols, engine);
        enclose_room(tilemap.tile_ids, rows, cols);
        keep_largest_open_area(tilemap.tile_ids, rows, cols);
        
        tilemaps[i] = tilemap;
    }


    // Post-process the map to ensure connectivity
    for (int i = 0; i < num_rooms; i++) {
        Point current_room = map_layout.room_locations[i];

        //Check Right Neighbor
        Point right_neighbor = { current_room.x + 1, current_room.y };
        if (get_room_index(map_layout, right_neighbor) != -1) {
            create_path_between_rooms(map_layout, tilemaps, current_room, right_neighbor);
        }

        //Check Up Neighbor
        Point up_neighbor = { current_room.x, current_room.y + 1 };
        if (get_room_index(map_layout, up_neighbor) != -1) {
            create_path_between_rooms(map_layout, tilemaps, current_room, up_neighbor);
        }

        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < cols; c++)
            {
                int idx = r * cols + c;
                if (tilemaps[i].tile_ids[idx] == WALL)
                    continue;

                if (random_up_to(100,engine) < 16)
                {
                    tilemaps[i].tile_ids[idx] = ALTERNATE_FLOOR;
                }
                
            }
            
        }
        
    }


    return map_layout;

}
