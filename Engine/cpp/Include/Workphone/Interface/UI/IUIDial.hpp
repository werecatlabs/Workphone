#ifndef IGUIDial_h__
#define IGUIDial_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class WPCore_API IUIDial : public IUIElement
        {
        public:
            IUIDial() : IUIElement( IUIDial::typeInfo() )
            {
            }

            IUIDial( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Virtual destructor. */
            ~IUIDial() override;

            virtual void setNeedlePosition( f32 position ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IGUIDial_h__
