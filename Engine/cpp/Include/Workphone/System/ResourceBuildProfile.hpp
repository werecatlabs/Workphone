#pragma once
#include <Workphone/Core/StringTypes.hpp>
#include <map>
#include <stdexcept>

namespace workphone::resource
{
    /** Captured semantic build inputs. Toolchain must identify the actual cooker/
     * encoder binaries; settings contain their effective, resolved options. */
    struct ResourceBuildProfile
    {
        String profile;
        String toolchain;
        std::map<String, String> settings;

        String key() const
        {
            if(profile.empty() && toolchain.empty() && settings.empty()) return {};
            if(profile.empty() || toolchain.empty() || settings.size()>64)
                throw std::invalid_argument("Build profile requires profile and toolchain identities and at most 64 settings");
            String result="workphone-build-v1";
            auto append=[&](const String &value) {
                if(value.size()>4096 || value.find('\0')!=String::npos)
                    throw std::invalid_argument("Invalid build profile field");
                result += ":" + String(std::to_string(value.size()).c_str()) + ":" + value;
            };
            append(profile); append(toolchain);
            for(const auto &setting:settings) {
                if(setting.first.empty()) throw std::invalid_argument("Empty build setting name");
                append(setting.first); append(setting.second);
            }
            return result;
        }
    };
}
