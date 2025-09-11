#pragma once

#include <nlohmann/json.hpp>

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

namespace tr {

struct TaskNode {
    std::string id;
    std::vector<std::string> dependsOn;
    std::string cmd;
    std::vector<std::string> args;
    std::unordered_map<std::string, std::string> env;
    std::string cwd;
    int retries{0};
    long long timeoutMs{0};
    bool ignoreFailure{false};
    std::vector<TaskNode> tasks;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TaskNode, id, dependsOn, cmd, args, env, cwd, retries, timeoutMs, ignoreFailure, tasks)
};

struct TaskSpec {
    std::string id;
    std::vector<std::string> dependsOn;
    std::string cmd;
    std::vector<std::string> args;
    std::unordered_map<std::string, std::string> env;
    std::string cwd;
    int retries{0};
    long long timeoutMs{0};
    bool ignoreFailure{false};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TaskSpec, id, dependsOn, cmd, args, env, cwd, retries, timeoutMs, ignoreFailure)
};

struct Config {
    std::vector<TaskNode> tasks;
    std::vector<std::string> entry;
    int parallel{4};
    bool stopOnFailure{true};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Config, tasks, entry, parallel, stopOnFailure)
};

Config parse_config(const nlohmann::json &root);

} 
