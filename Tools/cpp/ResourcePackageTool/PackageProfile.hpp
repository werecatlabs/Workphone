#pragma once
#include <Workphone/Workphone.hpp>

// Versioned CLI profiles use the same checked package request as positional input.
workphone::Array<workphone::String> loadPackageProfile(const workphone::String &path);
void savePackageProfile(const workphone::String &path,
                        const workphone::Array<workphone::String> &arguments);
