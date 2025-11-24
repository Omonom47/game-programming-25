#ifndef ITU_UNITY_BUILD
#include <itu_common.hpp>
#endif

struct Room 
{
    int* tiles;
    int num_rows;
    int num_cols;
};


Room generate_room_matrix_from_file(std::string file_path)
{

    std::ifstream f(file_path);

    if (!f.is_open()) {
        std::cerr << "Failed to open file." << std::endl;
        std::cerr << "File path: " << file_path << std::endl;
        std::string current_path = std::filesystem::current_path().string();
        std::cerr << "Current working directory: " << current_path << std::endl;
        return {};
    }

    std::vector<std::vector<int>> room;
    std::string line;

    while (std::getline(f, line)) {
        std::vector<int> row;
        for (char ch : line) {
            switch (ch) {
                case '0':
                    row.push_back(0);
                    break;
                case '1':
                    row.push_back(1);
                    break; 
                default:
                    std::cerr << "Unknown tile type: " << ch << std::endl;
                    break;
            }
        }
        room.push_back(row);
    }

    f.close();

    // inverting the room vertically to match the coordinate system
    std::reverse(room.begin(), room.end());

    // flatten 2D vector to 1D array
    int* flattened_room = new int[room.size() * room[0].size()];
    for (int y = 0; y < room.size(); ++y) {
        for (int x = 0; x < room[y].size(); ++x) 
        {
            flattened_room[y * room[0].size() + x] = room[y][x]; 
        }
    }

    Room room_def = {  };
    room_def.tiles = flattened_room;
    room_def.num_rows = (int)room.size();
    room_def.num_cols = (int)room[0].size();

    return room_def;
}




