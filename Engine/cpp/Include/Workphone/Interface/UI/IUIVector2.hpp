#ifndef IUIVector2_h__
#define IUIVector2_h__

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIVector2
         * @brief Interface for a 2D vector UI element, responsible for managing 2D vector elements in
         * the user interface
         */
        class WPCore_API IUIVector2 : public IUIElement
        {
        public:
            IUIVector2() : IUIElement( IUIVector2::typeInfo() )
            {
            }

            IUIVector2( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /**
             * @brief Virtual destructor
             */
            ~IUIVector2() override;

            /**
             * @brief Gets the value of the 2D vector element
             * @return The value of the 2D vector element as a Vector2 object
             */
            virtual Vector2<real_Num> getValue() const = 0;

            /**
             * @brief Sets the value of the 2D vector element
             * @param value The new value for the 2D vector element as a Vector2 object
             */
            virtual void setValue( const Vector2<real_Num> &value ) = 0;

            /**
             * @brief Gets the label text associated with the 2D vector element
             * @return The label text as a string
             */
            String getLabel() const override = 0;

            /**
             * @brief Sets the label text associated with the 2D vector element
             * @param label The new label text for the 2D vector element
             */
            void setLabel( const String &label ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUIVector2_h__
