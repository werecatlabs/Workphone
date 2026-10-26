#ifndef WP_MESH_IMPORT_STREAMING_HPP
#define WP_MESH_IMPORT_STREAMING_HPP

#include <Workphone/Scene/Directors/MeshResourceDirector.hpp>

namespace workphone::scene
{
    /** Builds the serialized prefab produced by a mesh import. Cooking uses a
     * private generation below project Cache; publishing the prefab is the
     * caller's transaction. Neither the input nor previous output arguments are
     * changed on failure. Disabled streaming returns an independent source copy. */
    class WPCore_API MeshImportStreaming final
    {
    public:
        static bool buildPrefab( const Properties &sourceActorData,
                                 const MeshResourceDirector::StreamingOptions &options,
                                 const String &projectRoot, const String &outputGenerationDirectory,
                                 SmartPtr<Properties> &prefabData, String &manifestPath, String &error );
    };
}  // namespace workphone::scene

#endif
