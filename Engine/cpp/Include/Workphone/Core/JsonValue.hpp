#pragma once

#include <Workphone/Core/StringTypes.hpp>
#include <variant>
#include <vector>
#include <map>
#include <string>

namespace workphone
{

    struct JsonValue;

    using JsonObject = std::map<String, JsonValue>;
    using JsonArray = std::vector<JsonValue>;

    struct WPCore_API JsonValue
        : std::variant<std::nullptr_t, bool, double, String, JsonArray, JsonObject>
    {
        using variant::variant;
    };

}  // namespace workphone
