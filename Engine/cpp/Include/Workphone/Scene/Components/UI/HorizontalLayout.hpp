#ifndef HorizontalLayoutContainer_h__
#define HorizontalLayoutContainer_h__

#include <Workphone/Scene/Components/UI/LayoutContainer.hpp>

namespace workphone
{
    namespace scene
    {
        /** Horizontal layout container. */
        class WPCore_API HorizontalLayout : public LayoutContainer
        {
        public:
            /** Constructor. */
            HorizontalLayout();

            /** Destructor. */
            ~HorizontalLayout() override;

            /** @copydoc LayoutContainer::updateTransform */
            void updateTransform() override;

            /** @copydoc LayoutContainer::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc LayoutContainer::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // HorizontalLayoutContainer_h__
