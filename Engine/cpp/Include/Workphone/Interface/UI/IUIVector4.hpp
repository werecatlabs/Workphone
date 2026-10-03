#ifndef IUIVector4_h__
#define IUIVector4_h__

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Math/Vector4.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIVector4
         * @brief Interface for a 4D vector UI element, responsible for managing 4D vector elements in
         * the user interface
         */
        class WPCore_API IUIVector4 : public IUIElement
        {
        public:
            IUIVector4() : IUIElement( IUIVector4::typeInfo() )
            {
            }

            IUIVector4( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /**
             * @brief Virtual destructor
             */
            ~IUIVector4() override;

            /**
             * @brief Gets the value of the 4D vector element
             * @return The value of the 4D vector element as a Vector4 object
             */
            virtual Vector4<real_Num> getValue() const = 0;

            /**
             * @brief Sets the value of the 4D vector element
             * @param value The new value for the 4D vector element as a Vector4 object
             */
            virtual void setValue( const Vector4<real_Num> &value ) = 0;

            /**
             * @brief Gets the label text associated with the 4D vector element
             * @return The label text as a string
             */
            String getLabel() const override = 0;

            /**
             * @brief Sets the label text associated with the 4D vector element
             * @param label The new label text for the 4D vector element
             */
            void setLabel( const String &label ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUIVector4_h__
