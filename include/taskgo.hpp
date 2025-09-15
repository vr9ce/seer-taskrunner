#pragma once
#include <bits/stdc++.h>
#include <generator>
#include "nlohmann/json.hpp"
using namespace std::literals;

namespace taskgo /* 可以换成别的名字 */ {

struct Scope: public std::enable_shared_from_this<Scope> {
    std::unordered_map<
        std::string,
        std::variant<
            std::monostate,
            double, std::string, bool
        >
    > vars;
    const std::shared_ptr<Scope> outer = nullptr;
};

class [[gnu::weak]] Interpreter {
    const ::nlohmann::json tree;
  public:
    Interpreter(const ::nlohmann::json &node): tree(node) {

    }


};

}  // namespace gogo
