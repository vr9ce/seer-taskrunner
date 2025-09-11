#include "Runner.h"

#include <csignal>
#include <filesystem>
#include <sstream>
#include <thread>
#include <iostream>

#ifdef __linux__
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

namespace tr {

using namespace std::chrono_literals;

Runner::Runner(TaskDefinition taskDef) : taskDef_(std::move(taskDef)) {}

bool Runner::looksAbsoluteId(const std::string &s) {
    return s.find('.') != std::string::npos || s.find(':') != std::string::npos || s.find('/') != std::string::npos;
}

void Runner::flatten_blocks(const Block& block, const std::string& prefix) {
    std::string blockId = prefix.empty() ? std::to_string(block.id) : (prefix + "." + std::to_string(block.id));
    flattened_.push_back(block);
    
    for (const auto& [key, childBlocks] : block.children) {
        for (const auto& child : childBlocks) {
            flatten_blocks(child, blockId);
        }
    }
}

void Runner::build_index() {
    flattened_.clear();
    for (const auto& [key, blocks] : taskDef_.rootBlock.children) {
        for (const auto& block : blocks) {
            flatten_blocks(block, "");
        }
    }
    
    id_to_index_.clear();
    for (size_t i = 0; i < flattened_.size(); ++i) {
        id_to_index_[std::to_string(flattened_[i].id)] = i;
    }
    statuses_.assign(flattened_.size(), TaskStatus::Pending);
    remaining_deps_.assign(flattened_.size(), 0);
}

void Runner::schedule_initial() {
    std::vector<char> needed(flattened_.size(), true);
    for (size_t i = 0; i < flattened_.size(); ++i) {
        if (statuses_[i] == TaskStatus::Pending && remaining_deps_[i] == 0) ready_.push(i);
    }
}

static std::string get_block_command(const Block &t) {
    std::ostringstream oss;
    oss << "Block[" << t.blockType << "] " << t.name;
    if (!t.inputParams.empty()) {
        oss << " (";
        bool first = true;
        for (const auto& [key, param] : t.inputParams) {
            if (!first) oss << ", ";
            oss << key << "=" << param.value;
            first = false;
        }
        oss << ")";
    }
    return oss.str();
}

TaskResult Runner::run_task(const Block &task) {
    TaskResult res;
    res.id = std::to_string(task.id);
    res.status = TaskStatus::Running;

    std::string command = get_block_command(task);
    std::cout << "Executing: " << command << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (task.blockType == "RobotSingleNavigationBp") {
        res.status = TaskStatus::Success;
        res.exit_code = 0;
        res.message = "Navigation completed";
    } else if (task.blockType == "RobotSingleUserDefinedBp") {
        res.status = TaskStatus::Success;
        res.exit_code = 0;
        res.message = "User defined action completed";
    } else if (task.blockType == "WhileBp") {
        res.status = TaskStatus::Success;
        res.exit_code = 0;
        res.message = "While loop completed";
    } else if (task.blockType == "RepeatNumBp") {
        res.status = TaskStatus::Success;
        res.exit_code = 0;
        res.message = "Repeat completed";
    } else {
        res.status = TaskStatus::Success;
        res.exit_code = 0;
        res.message = "Block executed";
    }
    
    return res;
}

void Runner::schedule_if_ready(size_t idx) {
    if (statuses_[idx] == TaskStatus::Pending && remaining_deps_[idx] == 0) {
        ready_.push(idx);
    }
}

int Runner::run() {
    build_index();
    schedule_initial();

    std::mutex print_mtx;
    std::vector<std::jthread> workers;
    std::atomic<bool> had_failure{false};

    auto worker = [&](std::stop_token st){
        while (!st.stop_requested()) {
            size_t idx = SIZE_MAX;
            {
                std::unique_lock lk(mtx_);
                cv_.wait(lk, [&]{ return !ready_.empty() || running_ == 0; });
                if (ready_.empty()) return;
                idx = ready_.front(); ready_.pop();
                statuses_[idx] = TaskStatus::Running;
                ++running_;
            }

            const Block task = flattened_[idx];
            int attempts = 1;
            TaskResult result;
            for (int a = 0; a < attempts; ++a) {
                result = run_task(task);
                if (result.status == TaskStatus::Success) break;
            }

            bool success = (result.status == TaskStatus::Success);
            {
                std::scoped_lock outlk(print_mtx);
                std::cout << (success ? "[OK] " : "[FAIL] ") << task.id << " (exit=" << result.exit_code << ")\n";
            }

            {
                std::lock_guard lk(mtx_);
                statuses_[idx] = success ? TaskStatus::Success : TaskStatus::Failed;
                --running_;

                if (!success) {
                    if (true && !task.disabled) {
                        std::queue<size_t> empty; std::swap(ready_, empty);
                        cv_.notify_all();
                        had_failure = true;
                        return;
                    }
                }

                // No dependencies in block structure
            }
            cv_.notify_all();
        }
    };

    int threads = std::max(1, 4);
    workers.reserve(threads);
    for (int i = 0; i < threads; ++i) {
        workers.emplace_back(worker);
    }
    for (auto &t : workers) t.join();

    return had_failure ? 1 : 0;
}

} 
