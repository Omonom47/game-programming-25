#ifndef ITU_UNITY_BUILD
#include <itu_common.hpp>
#endif

void benchmark_map_generation(SDLContext* context) 
{
	std::vector<int> room_sizes = {30, 50, 100, 200};
	std::vector<int> room_num = {5, 20, 50, 100};
	const int iterations = 10;

    std::string output_dir = "../exam_project/results/";
    try {
        if (!std::filesystem::exists(output_dir)) {
            std::filesystem::create_directories(output_dir);
            printf("Created directory: %s\n", output_dir.c_str());
        }
    } catch (const std::filesystem::filesystem_error& e) {
        printf("Warning: Could not verify directory %s. Error: %s\n", output_dir.c_str(), e.what());
    }

	std::ofstream csv_raw(output_dir + "map_benchmark_raw.csv");
	std::ofstream csv_summary(output_dir + "map_generation_benchmark.csv");
	if (!csv_raw.is_open() || !csv_summary.is_open()) {
		printf("Error: Could not open CSV file for writing.\n");
		return;
	}

	// CSV headers
	csv_raw << "Room_Size,Num_Rooms,Iteration,Time_ms\n";
    csv_summary << "Room_Size,Num_Rooms,Mean_ms,StdDev_ms\n";

	for (int size : room_sizes) {
		for (int num : room_num) {
			std::vector<double> times;
			times.reserve(iterations);

			for(int i = 0; i < iterations; ++i) {
				std::vector<Tilemap> rooms(num);

				auto start = std::chrono::high_resolution_clock::now();
				Map map = generate_map(rooms.data(), size, size, num, context->prng);
				auto end = std::chrono::high_resolution_clock::now();

				double ms = std::chrono::duration<double, std::milli>(end - start).count();
				times.push_back(ms);
				csv_raw << size << "," << num << "," << (i + 1) << "," << ms << "\n";

				// Cleanup (tile_ids)
				for (int r = 0; r < num; r++) {
					if (rooms[r].tile_ids) {
                        free(rooms[r].tile_ids);
                        rooms[r].tile_ids = nullptr;
                    }
				}
			}

			// Calculate Statistics
			double sum = std::accumulate(times.begin(), times.end(), 0.0);
            double mean = sum / times.size();

            double sq_sum = std::inner_product(times.begin(), times.end(), times.begin(), 0.0);
            double stdev = std::sqrt(sq_sum / times.size() - mean * mean);

			csv_summary << size << "," << num << "," << mean << "," << stdev << "\n";
		}
	}
	csv_raw.close();
    csv_summary.close();
	
}