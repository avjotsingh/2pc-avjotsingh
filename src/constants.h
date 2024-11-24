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
        { "S1", 1 },
        { "S2", 2 },
        { "S3", 3 },
        { "S4", 4 },
        { "S5", 5 },
        { "S6", 6 },
        { "S7", 7 },
        { "S8", 8 },
        { "S9", 9 }
    };
    std::map<std::string, int> cluster_ids = {
        { "S1", 1 },
        { "S2", 1 },
        { "S3", 1 },
        { "S4", 2 },
        { "S5", 2 },
        { "S6", 2 },
        { "S7", 3 },
        { "S8", 3 },
        { "S9", 3 }
    };
}