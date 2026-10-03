#pragma once

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a UI separator element.
         * A separator is typically used to visually divide different sections of a UI.
         */
        class WPCore_API IUISeparator : public IUIElement
        {
        public:
            IUISeparator();

            IUISeparator( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUISeparator() override;

            /** Sets the orientation of the separator.
             * @param horizontal True for horizontal separator, false for vertical.
             */
            virtual void setHorizontal( bool horizontal ) = 0;

            /** Gets the orientation of the separator.
             * @return True if horizontal, false if vertical.
             */
            virtual bool isHorizontal() const = 0;

            /** Sets the thickness of the separator line.
             * @param thickness The thickness in pixels.
             */
            virtual void setThickness( f32 thickness ) = 0;

            /** Gets the thickness of the separator line.
             * @return The thickness in pixels.
             */
            virtual f32 getThickness() const = 0;

            /** Sets the margin around the separator.
             * @param margin The margin in pixels.
             */
            virtual void setMargin( f32 margin ) = 0;

            /** Gets the margin around the separator.
             * @return The current margin in pixels.
             */
            virtual f32 getMargin() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone
