#ifndef _IGUIBar_H
#define _IGUIBar_H

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        class WPCore_API IUIBar : public IUIElement
        {
        public:
            enum class BarOrientation
            {
                BO_HORIZONTAL,
                BO_VERTICAL,

                BO_COUNT
            };

            IUIBar();

            IUIBar( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUIBar() override;

            /** The amount of points the bar has
            this represented as a whole number.
            */
            virtual void setPoints( f32 points ) = 0;

            /** Set the maximum amount of points to the bar can contain
            e.g. a player might have 250 health points.
            */
            virtual void setMaxPoints( f32 maxPoints ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif
