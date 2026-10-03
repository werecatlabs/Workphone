#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ComponentEvent.hpp>
#include <Workphone/Interface/Scene/IComponentEventListener.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone::scene
{
    ComponentEvent::ComponentEvent() = default;

    ComponentEvent::~ComponentEvent() = default;

    void ComponentEvent::addListener( SmartPtr<IComponentEventListener> listener )
    {
        if( !listener )
        {
            return;
        }

        if( std::find( m_listeners.begin(), m_listeners.end(), listener ) == m_listeners.end() )
        {
            m_listeners.push_back( listener );
        }
    }

    void ComponentEvent::removeListener( SmartPtr<IComponentEventListener> listener )
    {
        if( !listener )
        {
            return;
        }

        auto it = std::find( m_listeners.begin(), m_listeners.end(), listener );
        if( it != m_listeners.end() )
        {
            m_listeners.erase( it );
        }
    }

    void ComponentEvent::removeListeners()
    {
        m_listeners.clear();
    }

    auto ComponentEvent::getListeners() const -> Array<SmartPtr<IComponentEventListener>>
    {
        return m_listeners;
    }

    void ComponentEvent::setListeners( const Array<SmartPtr<IComponentEventListener>> &listeners )
    {
        m_listeners.clear();
        for( const auto &listener : listeners )
        {
            if( listener )
            {
                m_listeners.push_back( listener );
            }
        }
    }

    auto ComponentEvent::getLabel() const -> String
    {
        return m_label;
    }

    void ComponentEvent::setLabel( const String &label )
    {
        m_label = label;
    }

    auto ComponentEvent::toData() const -> SmartPtr<ISharedObject>
    {
        auto props = SmartPtr<Properties>( new Properties() );
        if( props )
        {
            props->setProperty( "label", m_label.c_str(), Properties::stringTypeStr );
            props->setProperty( "eventHash", std::to_string( m_eventHash ), Properties::intTypeStr );
            return props;
        }
        return nullptr;
    }

    void ComponentEvent::fromData( SmartPtr<ISharedObject> data )
    {
        if( !data )
        {
            return;
        }

        SmartPtr<Properties> props( static_cast<Properties *>( data.get() ) );
        if( props )
        {
            m_label = props->getProperty( "label", "" );

            String hashStr = props->getProperty( "eventHash", "0" );
            try
            {
                m_eventHash = static_cast<hash_type>( std::stoll( hashStr ) );
            }
            catch( const std::exception &e )
            {
                m_eventHash = 0;
            }
        }
    }
    auto ComponentEvent::getProperties() const -> SmartPtr<Properties>
    {
        return m_properties;
    }

    void ComponentEvent::setProperties( SmartPtr<Properties> properties )
    {
        m_properties = properties;
    }

    auto ComponentEvent::getEventHash() const -> hash_type
    {
        return m_eventHash;
    }

    void ComponentEvent::setEventHash( hash_type eventHash )
    {
        m_eventHash = eventHash;
    }
}  // namespace workphone::scene
