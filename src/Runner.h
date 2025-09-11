#pragma once

#include "Task.h"

#include <condition_variable>
#include <future>
#include <queue>
#include <set>

namespace tr {

enum class TaskStatus { Pending, Running, Success, Failed, Skipped };

struct TaskResult {
    TaskStatus status{TaskStatus::Pending};
    int exit_code{0};
    std::string id;
    std::string message;
};

class Runner {
public:
    explicit Runner(Config cfg);

    int run();

private:
    Config cfg_;
    std::vector<TaskSpec> flattened_;

    std::unordered_map<std::string, size_t> id_to_index_;
    std::vector<TaskStatus> statuses_;
    std::vector<int> remaining_deps_;

    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<size_t> ready_;
    size_t running_{0};

    struct Inherit {
        std::unordered_map<std::string, std::string> env;
        std::string cwd;
        int retries{0};
        long long timeoutMs{0};
        bool ignoreFailure{false};
        std::vector<std::string> depends;
    };

    static void mergeEnv(std::unordered_map<std::string, std::string> &base, const std::unordered_map<std::string, std::string> &over);
    static bool looksAbsoluteId(const std::string &s);
    void flatten_node(const TaskNode &node, const std::string &prefix, const Inherit &inherit);
    void flatten_all();

    void build_index();
    void schedule_initial();
    void schedule_if_ready(size_t idx);
    TaskResult run_task(const TaskSpec &task);
};

} 
