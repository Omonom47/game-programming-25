#include <bits/stdc++.h>
using namespace std;

struct Room 
{
    int* tiles;
    int num_rows;
    int num_cols;
};


Room generate_room_matrix_from_file(string file_path)
{
    ifstream f(file_path);

    if (!f.is_open()) {
        cerr << "Failed to open file." << endl;
        cerr << "File path: " << file_path << endl;
        string current_path = std::filesystem::current_path().string();
        cerr << "Current working directory: " << current_path << endl;
        return {};
    }

    vector<vector<int>> room;
    string line;

    while (getline(f, line)) {
        vector<int> row;
        for (char ch : line) {
            switch (ch) {
                case '0':
                    row.push_back(0);
                    break;
                case '1':
                    row.push_back(1);
                    break; 
                default:
                    cerr << "Unknown tile type: " << ch << endl;
                    break;
            }
        }
        room.push_back(row);
    }

    f.close();

    // inverting the room vertically to match the coordinate system
    reverse(room.begin(), room.end());

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




