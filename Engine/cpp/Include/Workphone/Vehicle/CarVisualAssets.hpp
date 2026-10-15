#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <string>

namespace workphone::vehiclevisual
{
    /// Convert meshopt-compressed GLBs to an importer-friendly cached GLB.
    /// Preserves the source, embedded images, scene transforms, and material data.
    /// Integer vertex attributes become float attributes for the native importer.
    WPCore_API std::string prepareCarVisualGLB( const std::string &sourcePath,
                                                const std::string &cacheDirectory );
}  // namespace workphone::vehiclevisual
