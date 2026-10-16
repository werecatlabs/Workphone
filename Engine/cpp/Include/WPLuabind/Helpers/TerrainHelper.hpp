#ifndef TerrainHelper_h__
#define TerrainHelper_h__

#include <Workphone/Workphone.hpp>

namespace workphone
{
    class TerrainHelper
    {
    public:
        static String brush( scene::TerrainSystem *terrain, const String &mode, f32 x, f32 z,
                             f32 radius, f32 strength, f32 target );
        static String save( scene::TerrainSystem *terrain, const String &path );
        static String load( scene::TerrainSystem *terrain, const String &path );
        static String importData( scene::TerrainSystem *terrain, const String &data );
    };
}  // namespace workphone

#endif  // TerrainHelper_h__
