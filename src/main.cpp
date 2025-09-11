#include "Task.h"
#include "Runner.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace tr;

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "task.json";
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Failed to open " << path << "\n";
        return 2;
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    try {
        nlohmann::json root = nlohmann::json::parse(content);
        Config cfg = parse_config(root);
        Runner runner(std::move(cfg));
        return runner.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
