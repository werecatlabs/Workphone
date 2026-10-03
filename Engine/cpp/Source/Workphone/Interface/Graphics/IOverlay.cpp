#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IOverlay.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IOverlay, ISharedObject );

    IOverlay::~IOverlay() = default;

    const hash_type IOverlay::STATE_MESSAGE_ATTACH_OBJECT = StringUtil::getHash( "attachObject" );
    const hash_type IOverlay::STATE_MESSAGE_DETACH_OBJECT = StringUtil::getHash( "detachObject" );
    const hash_type IOverlay::STATE_MESSAGE_DETACH_ALL_OBJECTS =
        StringUtil::getHash( "detachAllObject" );

}  // namespace workphone::render
