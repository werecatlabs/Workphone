#ifndef VerticalLayoutContainer_h__
#define VerticalLayoutContainer_h__

#include <Workphone/Scene/Components/UI/LayoutContainer.hpp>

namespace workphone
{
    namespace scene
    {
        /** Vertical layout container.
         *  This container will lay out its children vertically, from top to bottom.
         *  The width of the container will be the width of the widest child, and the height
         *  will be the sum of the heights of all children plus the spacing between them.
         */
        class WPCore_API VerticalLayout : public LayoutContainer
        {
        public:
            /** Constructor. */
            VerticalLayout();

            /** Destructor. */
            ~VerticalLayout() override;

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

#endif  // VerticalLayoutContainer_h__
