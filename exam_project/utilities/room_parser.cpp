#include <bits/stdc++.h>
using namespace std;


vector<vector<int>> generate_room_matrix_from_file(string file_path)
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

    return room;
}




