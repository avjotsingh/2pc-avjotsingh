#pragma once
#include <string.h>
#include <vector>
#include <map>

namespace constants {
    std::vector<std::string> server_names = { "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8", "S9" };
    std::map<std::string, std::string> server_addresses = {
        { "S1", "localhost:50051" },
        { "S2", "localhost:50052" },
        { "S3", "localhost:50053" },
        { "S4", "localhost:50054" },
        { "S5", "localhost:50055" },
        { "S6", "localhost:50056" },
        { "S7", "localhost:50057" },
        { "S8", "localhost:50058" },
        { "S9", "localhost:50059" }
    };
    std::map<std::string, int> server_ids = {
        { "S1", 0 },
        { "S2", 1 },
        { "S3", 2 },
        { "S4", 3 },
        { "S5", 4 },
        { "S6", 5 },
        { "S7", 6 },
        { "S8", 7 },
        { "S9", 8 }
    };
    std::map<std::string, int> cluster_ids = {
        { "S1", 0 },
        { "S2", 0 },
        { "S3", 0 },
        { "S4", 1 },
        { "S5", 1 },
        { "S6", 1 },
        { "S7", 2 },
        { "S8", 2 },
        { "S9", 2 }
    };
    std::map<std::string, int> client_clusters = {
        { "A", 0 },
        { "B", 1 },
        { "C", 2 },
        { "D", 3 },
        { "E", 4 },
        { "F", 5 },
        { "G", 6 },
        { "H", 7 },
        { "I", 8 }
    };
}