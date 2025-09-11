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
    explicit Runner(TaskDefinition taskDef);

    int run();

private:
    TaskDefinition taskDef_;
    std::vector<Block> flattened_;

    std::unordered_map<std::string, size_t> id_to_index_;
    std::vector<TaskStatus> statuses_;
    std::vector<int> remaining_deps_;

    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<size_t> ready_;
    size_t running_{0};

    static bool looksAbsoluteId(const std::string &s);

    void flatten_blocks(const Block& block, const std::string& prefix);
    void build_index();
    void schedule_initial();
    void schedule_if_ready(size_t idx);
    TaskResult run_task(const Block &task);
};

} 
