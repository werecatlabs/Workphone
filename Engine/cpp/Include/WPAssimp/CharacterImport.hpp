#pragma once

#include <WPAssimp/WPAssimpPrerequisites.hpp>
#include <Workphone/System/ResourceID.hpp>
#include <vector>

namespace workphone::character_import
{
    struct Sources
    {
        std::vector<u8> mesh;
        String skeleton;
        String skin;
    };

    /// Convert one skinned model into WPM1 geometry, skeleton/clip XML and a
    /// skin descriptor for the existing typed compilers. Requires a rigid
    /// bind hierarchy and up to four weights per vertex. Output is replaced
    /// only after the complete set passes validation.
    WPAssimp_API bool importSources( const String &inputPath, const resource::ResourceID &meshId,
                                    const resource::ResourceID &skeletonId, Sources &output,
                                    String &error );
}
