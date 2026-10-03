#ifndef __Vehicle_Types_h__
#define __Vehicle_Types_h__

#include <Workphone/WorkphoneTypes.hpp>

namespace workphone
{
    class Types
    {
    public:
        static const hash_type INPUT_UP;
        static const hash_type INPUT_DOWN;
        static const hash_type INPUT_RIGHT;
        static const hash_type INPUT_LEFT;

        static const u32 RESERVED_MASK = ( 0 << 31 );
        static const u32 SCENE_OBJECTS_MASK = ( 0 << 1 );
        static const u32 NPC_CUBE_MAP_MASK = ( 0 << 2 );
        static const u32 FOLIAGE_MASK = ( 0 << 3 );
    };
} // namespace fb

#endif  // __Vehicle_Types_h__
