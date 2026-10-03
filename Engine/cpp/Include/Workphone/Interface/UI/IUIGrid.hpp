#ifndef IUIGrid_h__
#define IUIGrid_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a ui frame. */
        class WPCore_API IUIGrid : public IUIElement
        {
        public:
            IUIGrid() : IUIElement( IUIGrid::typeInfo() )
            {
            }

            IUIGrid( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Destructor. */
            ~IUIGrid() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IUIGrid_h__
