#ifndef _COverlayElementContainer_H
#define _COverlayElementContainer_H

#include <WPGraphics/ClawOverlayElement.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementContainer.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class COverlayElementContainer
         * @brief Base implementation of an overlay element container.
         */
        class ClawOverlayElementContainer : public ClawOverlayElement<IOverlayElementContainer>
        {
        public:
            ClawOverlayElementContainer();
            ~ClawOverlayElementContainer() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
