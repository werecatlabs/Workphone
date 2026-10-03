#ifndef _WPGraphicsOgreNextTypes_H_
#define _WPGraphicsOgreNextTypes_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>

namespace workphone
{

    class GraphicsOgreNextTypes
    {
    public:
        enum class SceneManagerType
        {
            SMT_GENERIC,
            SMT_OCTTREE,
            SMT_TERRAIN,

            SMT_COUNT
        };

        enum class CoordinateSystemType
        {
            CST_LEFT_HANDED,
            CST_RIGHT_HANDED,

            CST_COUNT
        };

        static const hash_type FRAME_EVENT_PRE_RENDER;
        static const hash_type FRAME_EVENT_RENDER_QUEUED;
        static const hash_type FRAME_EVENT_POST_RENDER;
        static const hash_type VISIBILITY_MASK_HASH;
        static const hash_type STATE_MESSAGE_CAST_SHADOWS;
        static const hash_type STATE_MESSAGE_RENDER_QUEUE;
        static const hash_type STATE_MESSAGE_ATTENUATION;
        static const hash_type STATE_MESSAGE_DIRECTION;
    };

}  // namespace workphone

#endif
