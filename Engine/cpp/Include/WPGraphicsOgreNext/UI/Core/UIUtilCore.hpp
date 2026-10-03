#ifndef UIUtilCore_h__
#define UIUtilCore_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>

extern "C" {
#include <workphone_color.h>
#include <workphone_style.h>
}

struct wp_rect;

namespace workphone
{
    namespace ui
    {

        class UIUtilCore
        {
        public:
            static void calculatePositionAndSize( const Vector2F &relativePosition,
                                                  const Vector2F &relativeSize,
                                                  Vector2F &outAbsolutePosition,
                                                  Vector2F &outAbsoluteSize );

            static void calculateBounds( const Vector2F &position, const Vector2F &size,
                                         wp_rect *outBounds );

            static wp_color toWpColor( const ColourF &c );

            static wp_style_item solidItem( const ColourF &c );
        };

    }  // namespace ui
}  // namespace workphone
#endif  // UIUtilCore_h__
