#ifndef IUICheckbox_h__
#define IUICheckbox_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class WPCore_API IUICheckbox : public IUIElement
        {
        public:
            IUICheckbox() : IUIElement( IUICheckbox::typeInfo() )
            {
            }

            IUICheckbox( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Virtual destructor. */
            ~IUICheckbox() override;

            virtual void setValue( bool value ) = 0;
            virtual bool getValue() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IGUICheckbox_h__
