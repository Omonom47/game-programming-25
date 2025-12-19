#ifndef ITU_UNITY_BUILD
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
    int largest_area_size = 0;

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

void create_path_between_rooms(Map map, Point room_a, Point room_b) {

}


Map generate_map(Tilemap* tilemaps, int width, int height, int num_rooms, PRNG* engine) {
    Map map_layout = generate_layout(width, height, num_rooms, engine);

    const int rows = 30; 
    const int cols = 30;
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

    return map_layout;

}
