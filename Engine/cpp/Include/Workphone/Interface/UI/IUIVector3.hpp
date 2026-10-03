#ifndef __IUIVector3_h__
#define __IUIVector3_h__

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIVector3
         * @brief Interface for a 3D vector UI element, responsible for managing 3D vector elements in
         * the user interface
         */
        class WPCore_API IUIVector3 : public IUIElement
        {
        public:
            IUIVector3() : IUIElement( IUIVector3::typeInfo() )
            {
            }

            IUIVector3( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /**
             * @brief Virtual destructor
             */
            ~IUIVector3() override;

            /**
             * @brief Gets the value of the 3D vector element
             * @return The value of the 3D vector element as a Vector3 object
             */
            virtual Vector3<real_Num> getValue() const = 0;

            /**
             * @brief Sets the value of the 3D vector element
             * @param value The new value for the 3D vector element as a Vector3 object
             */
            virtual void setValue( const Vector3<real_Num> &value ) = 0;

            /**
             * @brief Gets the label text associated with the 3D vector element
             * @return The label text as a string
             */
            String getLabel() const override = 0;

            /**
             * @brief Sets the label text associated with the 3D vector element
             * @param label The new label text for the 3D vector element
             */
            void setLabel( const String &label ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // __IUIVector3_h__
