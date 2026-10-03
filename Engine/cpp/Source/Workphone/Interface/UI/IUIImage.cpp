#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIImage, IUIElement );

    IUIImage::IUIImage( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIImage::IUIImage() : IUIElement( IUIImage::typeInfo() )
    {
    }

    // stIatic property key definitions
    const String IUIImage::borderLeftStr = String( "borderLeft" );
    const String IUIImage::borderRightStr = String( "borderRight" );
    const String IUIImage::borderTopStr = String( "borderTop" );
    const String IUIImage::borderBottomStr = String( "borderBottom" );
    const String IUIImage::useTilingStr = String( "useTiling" );
    const String IUIImage::spriteSizeStr = String( "spriteSize" );
    const String IUIImage::materialStr = String( "material" );
    const String IUIImage::textureStr = String( "texture" );
    const String IUIImage::useNineSliceStr = String( "useNineSlice" );
    const String IUIImage::tileScaleXStr = String( "tileScaleX" );
    const String IUIImage::tileScaleYStr = String( "tileScaleY" );
    const String IUIImage::referenceWidthStr = String( "referenceWidth" );
    const String IUIImage::referenceHeightStr = String( "referenceHeight" );
    const String IUIImage::colourStr = String( "colour" );

    IUIImage::~IUIImage() = default;

}  // namespace workphone::ui
