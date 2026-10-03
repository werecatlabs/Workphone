#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/VerticalLayout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Scene/UiUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, VerticalLayout, LayoutContainer );

    VerticalLayout::VerticalLayout() = default;

    VerticalLayout::~VerticalLayout() = default;

    void VerticalLayout::updateTransform()
    {
        ScopedLock lock( this );

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
                    if( childVerticalAlignment == VerticalAlignment::TOP )
                    {
                        if( !children.empty() )
                        {
                            auto firstChild = children.front();
                            if( auto layoutTransform = firstChild->getComponent<LayoutTransform>() )
                            {
                                auto firstChildSize = layoutTransform->getSize();
                                offset += firstChildSize.y * 0.5f;
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

                            // Position the child vertically
                            auto childPosition = transform->getPosition();
                            auto childSize = transform->getSize();

                            childPosition.y = offset;
                            transform->setPosition( childPosition );

                            // Update the offset for the next child
                            offset += childSize.y + m_spacing;

                            child->updateTransform();
                        }
                    }
                }
            }
        }
    }

    SmartPtr<Properties> VerticalLayout::getProperties() const
    {
        auto properties = LayoutContainer::getProperties();

        return properties;
    }

    void VerticalLayout::setProperties( SmartPtr<Properties> properties )
    {
        LayoutContainer::setProperties( properties );
    }

}  // namespace workphone::scene
