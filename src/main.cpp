#include "Task.h"
#include "Runner.h"

#include <fstream>
#include <iostream>
#include <functional>
#include <nlohmann/json.hpp>

using namespace tr;

int main(int, const char *const argv[]) {
    auto in = std::ifstream{argv[1]};
    const auto content = std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};

    try {
        auto root = nlohmann::json::parse(content);

        // Handle null values by converting them to default values
        std::function<void(nlohmann::json&)> handleNulls = [&](nlohmann::json& j) {
            if (j.is_object()) {
                for (auto& [key, value] : j.items()) {
                    if (value.is_null()) {
                        if (key == "remark" || key == "color" || key == "refTaskDefId" || key == "ref") {
                            value = "";
                        } else if (key == "disabled") {
                            value = false;
                        }
                    } else if (value.is_object() && key == "inputParams") {
                        // Handle inputParams with mixed types
                        for (auto& [paramKey, paramValue] : value.items()) {
                            if (paramValue.is_object() && paramValue.contains("value")) {
                                if (paramValue["value"].is_boolean()) {
                                    paramValue["value"] = paramValue["value"].get<bool>() ? "true" : "false";
                                } else if (paramValue["value"].is_number()) {
                                    paramValue["value"] = std::to_string(paramValue["value"].get<long long>());
                                }
                            }
                        }
                        handleNulls(value);
                    } else if (value.is_object()) {
                        handleNulls(value);
                    } else if (value.is_array()) {
                        for (auto& item : value) {
                            if (item.is_object()) {
                                handleNulls(item);
                            }
                        }
                    }
                }
            }
        };

        handleNulls(root);
        TaskDefinition taskDef = root.get<TaskDefinition>();
        Runner runner(std::move(taskDef));
        return runner.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
