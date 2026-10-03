#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIImageArray.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    ClawUIImageArray::ClawUIImageArray()
    {
        setType( "ImageArray" );
    }

    ClawUIImageArray::~ClawUIImageArray() = default;

    SmartPtr<IUIImage> ClawUIImageArray::getImage( u32 index )
    {
        if( index < m_images.size() )
        {
            return m_images[index];
        }

        auto images = getChildrenByType<IUIImage>();
        return index < images.size() ? images[index] : nullptr;
    }

    void ClawUIImageArray::draw( struct wp_context *ctx )
    {
        if( !ctx || !isVisible() || !isEnabled() )
        {
            return;
        }

        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
