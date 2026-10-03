#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IFont, IResource );

    IFont::IFont( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    IFont::IFont() : IResource( IFont::typeInfo() )
    {
    }

    const String IFont::fontTypeStr = String( "font_type" );
    const String IFont::fontSourceStr = String( "font_source" );
    const String IFont::fontSizeStr = String( "font_size" );
    const String IFont::fontResolutionStr = String( "font_resolution" );

    IFont::~IFont() = default;

}  // namespace workphone::render
