#include "Task.h"

#include <stdexcept>

namespace tr {

using nlohmann::json;

Config parse_config(const json &root) {
    return root.get<Config>();
}

} 
