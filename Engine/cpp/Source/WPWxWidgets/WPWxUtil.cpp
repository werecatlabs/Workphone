#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/WPWxUtil.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        SmartPtr<render::IWindow> WxUtil::createRenderWindow( wxWindow *window, const String &name,
                                                              int width, int height,
                                                              const SmartPtr<Properties> &properties )
        {
            WP_ASSERT( window );
            WP_ASSERT( properties );

            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            if( graphicsSystem )
            {
                return graphicsSystem->createRenderWindow( name, width, height, false, properties );
            }
        }

    }  // namespace ui
}  // namespace workphone
