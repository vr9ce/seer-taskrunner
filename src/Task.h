#pragma once

#include <nlohmann/json.hpp>

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

namespace tr {

struct ParamValue {
    std::string type;
    std::string value;
    std::string ref;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ParamValue, type, value, ref)
};

struct Block {
    int id;
    std::string name;
    std::string blockType;
    std::string refTaskDefId;
    std::string remark;
    bool disabled{false};
    std::unordered_map<std::string, ParamValue> inputParams;
    std::unordered_map<std::string, std::vector<Block>> children;
    std::string color;
    bool selected{false};
    bool moving{false};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Block, id, name, blockType, refTaskDefId, remark, disabled, inputParams, children, color, selected, moving)
};

struct RootBlock {
    int id;
    std::string name;
    std::string blockType;
    std::string refTaskDefId;
    std::string remark;
    bool disabled{false};
    std::unordered_map<std::string, ParamValue> inputParams;
    std::unordered_map<std::string, std::vector<Block>> children;
    std::string color;
    bool selected{false};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(RootBlock, id, name, blockType, refTaskDefId, remark, disabled, inputParams, children, color, selected)
};

struct TaskDefinition {
    std::string label;
    bool builtin{false};
    std::vector<std::string> inputParams;
    std::vector<std::string> outputParams;
    std::vector<std::string> taskVariables;
    RootBlock rootBlock;
    long long modifiedOn{0};
    std::string modifiedBy;
    long long createdOn{0};
    std::string createdBy;
    std::string group;
    std::unordered_map<std::string, bool> blockChildrenCollapsed;
    std::vector<std::string> globalVariables;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TaskDefinition, label, builtin, inputParams, outputParams, taskVariables, rootBlock, modifiedOn, modifiedBy, createdOn, createdBy, group, blockChildrenCollapsed, globalVariables)
};

} // namespace tr