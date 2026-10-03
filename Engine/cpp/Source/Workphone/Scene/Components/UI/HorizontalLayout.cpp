#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/HorizontalLayout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Scene/UiUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, HorizontalLayout, LayoutContainer );

    HorizontalLayout::HorizontalLayout() = default;
    HorizontalLayout::~HorizontalLayout() = default;

    void HorizontalLayout::updateTransform()
    {
        if( isEnabled() )
        {
            auto offset = getOffset();
            auto childHorizontalAlignment = getChildHorizontalAlignment();
            auto childVerticalAlignment = getChildVerticalAlignment();

            if( auto actor = getActor() )
            {
                auto children = actor->getChildren();

                if( getUseChildStartOffset() )
                {
                    if( childHorizontalAlignment == HorizontalAlignment::LEFT )
                    {
                        if( !children.empty() )
                        {
                            auto firstChild = children.front();
                            if( auto layoutTransform = firstChild->getComponent<LayoutTransform>() )
                            {
                                auto firstChildSize = layoutTransform->getSize();
                                offset += firstChildSize.x * 0.5f;
                            }
                        }
                    }
                }

                for( auto child : children )
                {
                    if( child->isEnabled() )
                    {
                        auto transform = child->getComponent<LayoutTransform>();
                        if( transform )
                        {
                            if( getUseChildVerticalAlignment() )
                            {
                                transform->setVerticalAlignment( childVerticalAlignment );
                            }

                            if( getUseChildHorizontalAlignment() )
                            {
                                transform->setHorizontalAlignment( childHorizontalAlignment );
                            }

                            // Position the child
                            auto childPosition = transform->getPosition();
                            auto childSize = transform->getSize();

                            childPosition.x = offset;
                            transform->setPosition( childPosition );

                            // Update the offset for the next child
                            offset += childSize.x + m_spacing;

                            child->updateTransform();
                        }
                    }
                }
            }
        }
    }

    SmartPtr<Properties> HorizontalLayout::getProperties() const
    {
        auto properties = LayoutContainer::getProperties();

        return properties;
    }

    void HorizontalLayout::setProperties( SmartPtr<Properties> properties )
    {
        LayoutContainer::setProperties( properties );
    }

}  // namespace workphone::scene
