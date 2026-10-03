#ifndef _IOverlayElementContainer_H
#define _IOverlayElementContainer_H

#include <Workphone/Interface/Graphics/IOverlayElement.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for an overlay element container.
         */
        class WPCore_API IOverlayElementContainer : public IOverlayElement
        {
        public:
            /** Virtual destructor. */
            ~IOverlayElementContainer() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
