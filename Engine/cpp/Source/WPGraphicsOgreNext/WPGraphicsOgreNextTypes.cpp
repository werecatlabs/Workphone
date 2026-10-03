#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNextTypes.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{

    const hash_type GraphicsOgreNextTypes::FRAME_EVENT_PRE_RENDER = StringUtil::getHash( "pre_render" );
    const hash_type GraphicsOgreNextTypes::FRAME_EVENT_RENDER_QUEUED =
        StringUtil::getHash( "render_queued" );
    const hash_type GraphicsOgreNextTypes::FRAME_EVENT_POST_RENDER =
        StringUtil::getHash( "post_render" );

    const hash_type GraphicsOgreNextTypes::VISIBILITY_MASK_HASH =
        StringUtil::getHash( "visibilityMask" );
    const hash_type GraphicsOgreNextTypes::STATE_MESSAGE_CAST_SHADOWS =
        StringUtil::getHash( "cast_shadows" );
    const hash_type GraphicsOgreNextTypes::STATE_MESSAGE_RENDER_QUEUE =
        StringUtil::getHash( "renderQueue" );

    const hash_type GraphicsOgreNextTypes::STATE_MESSAGE_ATTENUATION =
        StringUtil::getHash( "attenuation" );
    const hash_type GraphicsOgreNextTypes::STATE_MESSAGE_DIRECTION = StringUtil::getHash( "direction" );

}  // namespace workphone
