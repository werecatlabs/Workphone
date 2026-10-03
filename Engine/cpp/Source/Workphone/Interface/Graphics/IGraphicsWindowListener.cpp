#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsWindowListener, ISharedObject );

    const hash_type IGraphicsWindowListener::windowClosingHash = StringUtil::getHash( "windowClosing" );
    const hash_type IGraphicsWindowListener::windowResizedHash = StringUtil::getHash( "windowResized" );
    const hash_type IGraphicsWindowListener::windowMovedHash = StringUtil::getHash( "windowMoved" );

    IGraphicsWindowListener::~IGraphicsWindowListener() = default;

}  // namespace workphone::render
