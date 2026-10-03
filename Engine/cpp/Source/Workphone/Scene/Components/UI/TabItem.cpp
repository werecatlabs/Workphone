#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/TabItem.hpp>
#include <Workphone/Scene/Components/UI/UIComponent.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    const String TabItem::labelStr = String( "Label" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, TabItem, UIComponent );

    TabItem::TabItem() = default;

    TabItem::~TabItem()
    {
    }

    void TabItem::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            UIComponent::load( data );

            createUI();
            updateElementState();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TabItem::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto state = getLoadingState();
            if( state == LoadingState::Unloaded || state == LoadingState::Unloading )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            m_textComponent = nullptr;
            m_content = nullptr;

            UIComponent::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Array<SmartPtr<ISharedObject>> TabItem::getChildObjects() const
    {
        auto children = UIComponent::getChildObjects();

        if( m_content )
        {
            children.push_back( m_content );
        }

        return children;
    }

    SmartPtr<Properties> TabItem::getProperties() const
    {
        auto props = UIComponent::getProperties();
        if( !props )
        {
            return nullptr;
        }

        props->setProperty( labelStr, m_label );

        return props;
    }

    void TabItem::setProperties( SmartPtr<Properties> properties )
    {
        if( properties )
        {
            String label = m_label;
            if( properties->getPropertyValue( labelStr, label ) )
            {
                setLabel( label );
            }
        }

        UIComponent::setProperties( properties );
    }

    bool TabItem::isValid() const
    {
        return !m_label.empty() && UIComponent::isValid();
    }

    void TabItem::createUI()
    {
        try
        {
            auto actor = getActor();
            if( !actor )
            {
                return;
            }

            // Resolve the child Text component used to display the label
            if( !m_textComponent )
            {
                // First try a named child actor "Label"
                if( auto labelActor = actor->findChild( "Label" ) )
                {
                    m_textComponent = labelActor->getComponent<Text>();
                }

                // Fall back to any Text component on this actor
                if( !m_textComponent )
                {
                    m_textComponent = actor->getComponent<Text>();
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TabItem::updateElementState()
    {
        try
        {
            syncLabelToText();
            updateVisibility();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TabItem::syncLabelToText()
    {
        if( m_textComponent )
        {
            if( m_textComponent->getText() != m_label )
            {
                m_textComponent->setText( m_label );
            }
        }
    }

    void TabItem::setLabel( const String &label )
    {
        if( m_label != label )
        {
            m_label = label;
            updateElementState();
        }
    }

    const String &TabItem::getLabel() const
    {
        return m_label;
    }

    void TabItem::setContent( SmartPtr<ISharedObject> content )
    {
        if( m_content != content )
        {
            m_content = content;
        }
    }

    SmartPtr<ISharedObject> TabItem::getContent() const
    {
        return m_content;
    }

}  // namespace workphone::scene
