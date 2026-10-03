#ifndef ClawUIList_h__
#define ClawUIList_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIAnimatedMaterial.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIList : public ClawUIElement<IUIElement>
        {
        public:
            void draw( struct wp_context *ctx ) override;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ClawUIList_h__
