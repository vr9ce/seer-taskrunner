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

Runner::Runner(Config cfg) : cfg_(std::move(cfg)) {}

void Runner::mergeEnv(std::unordered_map<std::string, std::string> &base, const std::unordered_map<std::string, std::string> &over) {
    for (const auto &kv : over) base[kv.first] = kv.second;
}

bool Runner::looksAbsoluteId(const std::string &s) {
    return s.find('.') != std::string::npos || s.find(':') != std::string::npos || s.find('/') != std::string::npos;
}

void Runner::flatten_node(const TaskNode &node, const std::string &prefix, const Inherit &inherit) {
    std::string fqid = prefix.empty() ? node.id : (prefix + "." + node.id);

    Inherit local = inherit;
    mergeEnv(local.env, node.env);
    if (!node.cwd.empty()) local.cwd = node.cwd;
    if (node.retries) local.retries = node.retries;
    if (node.timeoutMs > 0) local.timeoutMs = node.timeoutMs;
    if (node.ignoreFailure) local.ignoreFailure = node.ignoreFailure;
    local.depends.insert(local.depends.end(), node.dependsOn.begin(), node.dependsOn.end());

    if (!node.tasks.empty() && node.cmd.empty()) {
        for (const auto &child : node.tasks) flatten_node(child, fqid, local);
        return;
    }

    if (node.cmd.empty()) throw std::runtime_error("leaf task requires cmd: " + fqid);
    TaskSpec t;
    t.id = fqid;
    t.cmd = node.cmd;
    t.args = node.args;
    t.env = local.env;
    t.cwd = local.cwd;
    t.retries = local.retries;
    t.timeoutMs = local.timeoutMs;
    t.ignoreFailure = local.ignoreFailure;

    std::vector<std::string> deps = local.depends;
    for (auto &d : deps) if (!looksAbsoluteId(d) && !prefix.empty()) d = prefix + "." + d;
    t.dependsOn = std::move(deps);

    flattened_.push_back(std::move(t));
}

void Runner::flatten_all() {
    flattened_.clear();
    Inherit base;
    for (const auto &n : cfg_.tasks) flatten_node(n, "", base);
}

void Runner::build_index() {
    id_to_index_.clear();
    for (size_t i = 0; i < flattened_.size(); ++i) {
        id_to_index_[flattened_[i].id] = i;
    }
    statuses_.assign(flattened_.size(), TaskStatus::Pending);
    remaining_deps_.assign(flattened_.size(), 0);
    for (size_t i = 0; i < flattened_.size(); ++i) {
        remaining_deps_[i] = static_cast<int>(flattened_[i].dependsOn.size());
    }
}

void Runner::schedule_initial() {
    std::vector<char> needed(flattened_.size(), cfg_.entry.empty());
    std::function<void(size_t)> mark = [&](size_t idx){
        if (needed[idx]) return;
        needed[idx] = true;
        for (const auto &depId : flattened_[idx].dependsOn) {
            auto it = id_to_index_.find(depId);
            if (it != id_to_index_.end()) mark(it->second);
        }
    };
    for (const auto &id : cfg_.entry) {
        auto it = id_to_index_.find(id);
        if (it != id_to_index_.end()) mark(it->second);
    }
    for (size_t i = 0; i < flattened_.size(); ++i) {
        if (!needed[i]) statuses_[i] = TaskStatus::Skipped;
    }
    for (size_t i = 0; i < flattened_.size(); ++i) {
        if (statuses_[i] == TaskStatus::Pending && remaining_deps_[i] == 0) ready_.push(i);
    }
}

static std::string join_cmd(const TaskSpec &t) {
    std::ostringstream oss;
    oss << t.cmd;
    for (const auto &a : t.args) oss << ' ' << a;
    return oss.str();
}

TaskResult Runner::run_task(const TaskSpec &task) {
    TaskResult res;
    res.id = task.id;
    res.status = TaskStatus::Running;

#ifdef __linux__
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(task.cmd.c_str()));
    std::vector<std::string> owned_args = task.args;
    for (auto &s : owned_args) argv.push_back(const_cast<char*>(s.c_str()));
    argv.push_back(nullptr);

    std::vector<std::string> env_kv_owned;
    std::vector<char*> envp;
    for (char **e = environ; *e; ++e) envp.push_back(*e);
    for (const auto &kv : task.env) {
        env_kv_owned.push_back(kv.first + "=" + kv.second);
        envp.push_back(const_cast<char*>(env_kv_owned.back().c_str()));
    }
    envp.push_back(nullptr);

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    pid_t pid = 0;
    int spawn_res = 0;
    std::string cwd = task.cwd;
    if (!cwd.empty()) {
        std::string command = "cd '" + cwd + "' && " + join_cmd(task);
        std::vector<char*> sh_argv;
        sh_argv.push_back(const_cast<char*>("/bin/sh"));
        sh_argv.push_back(const_cast<char*>("-c"));
        std::string command_owned = command;
        sh_argv.push_back(const_cast<char*>(command_owned.c_str()));
        sh_argv.push_back(nullptr);
        spawn_res = posix_spawnp(&pid, "/bin/sh", &actions, nullptr, sh_argv.data(), envp.data());
    } else {
        spawn_res = posix_spawnp(&pid, task.cmd.c_str(), &actions, nullptr, argv.data(), envp.data());
    }
    posix_spawn_file_actions_destroy(&actions);
    if (spawn_res != 0) {
        res.status = TaskStatus::Failed;
        res.exit_code = spawn_res;
        res.message = "spawn failed";
        return res;
    }

    auto deadline = task.timeoutMs > 0 ? std::chrono::steady_clock::now() + std::chrono::milliseconds(task.timeoutMs) : std::chrono::time_point<std::chrono::steady_clock>::max();

    int status = 0;
    while (true) {
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) break;
        if (w == -1) {
            res.status = TaskStatus::Failed;
            res.exit_code = -1;
            res.message = "waitpid failed";
            return res;
        }
        if (std::chrono::steady_clock::now() > deadline) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            res.status = TaskStatus::Failed;
            res.exit_code = 137;
            res.message = "timeout";
            return res;
        }
        std::this_thread::sleep_for(10ms);
    }
    if (WIFEXITED(status)) {
        res.exit_code = WEXITSTATUS(status);
        res.status = (res.exit_code == 0) ? TaskStatus::Success : TaskStatus::Failed;
    } else if (WIFSIGNALED(status)) {
        res.exit_code = 128 + WTERMSIG(status);
        res.status = TaskStatus::Failed;
    } else {
        res.exit_code = -1;
        res.status = TaskStatus::Failed;
    }
#else
    res.status = TaskStatus::Failed;
    res.exit_code = -1;
    res.message = "Unsupported platform";
#endif
    return res;
}

void Runner::schedule_if_ready(size_t idx) {
    if (statuses_[idx] == TaskStatus::Pending && remaining_deps_[idx] == 0) {
        ready_.push(idx);
    }
}

int Runner::run() {
    flatten_all();
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

            const TaskSpec task = flattened_[idx];
            int attempts = std::max(1, task.retries + 1);
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
                    if (cfg_.stopOnFailure && !task.ignoreFailure) {
                        std::queue<size_t> empty; std::swap(ready_, empty);
                        cv_.notify_all();
                        had_failure = true;
                        return;
                    }
                }

                for (size_t j = 0; j < flattened_.size(); ++j) {
                    for (const auto &depId : flattened_[j].dependsOn) {
                        if (depId == task.id) {
                            --remaining_deps_[j];
                            if (!success && !flattened_[j].ignoreFailure) {
                                statuses_[j] = TaskStatus::Skipped;
                            }
                            if (statuses_[j] == TaskStatus::Pending) schedule_if_ready(j);
                        }
                    }
                }
            }
            cv_.notify_all();
        }
    };

    int threads = std::max(1, cfg_.parallel);
    workers.reserve(threads);
    for (int i = 0; i < threads; ++i) {
        workers.emplace_back(worker);
    }
    for (auto &t : workers) t.join();

    return had_failure ? 1 : 0;
}

} 
