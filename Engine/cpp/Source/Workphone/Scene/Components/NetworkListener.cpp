#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/NetworkListener.hpp>
#include <Workphone/Interface/Net/IPacket.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, NetworkListener, INetworkListener );

    NetworkListener::NetworkListener() = default;

    NetworkListener::~NetworkListener()
    {
        clearListeners();
    }

    // -------------------------------------------------------------------------
    // INetworkListener dispatch
    // -------------------------------------------------------------------------

    void NetworkListener::handlePacket( SmartPtr<IPacket> packet )
    {
        auto listeners = m_listeners.snapshot();
        for( auto &listener : listeners )
        {
            if( !listener )
            {
                continue;
            }

            try
            {
                // Each callback target gets an independent parse starting at the
                // message envelope. One listener must not consume data for the next.
                if( packet )
                {
                    packet->resetReadPointer();
                }
                listener->handlePacket( packet );
            }
            catch( std::exception &e )
            {
                // A malformed or unrelated packet must not prevent other callback
                // targets from seeing the event.
                WP_LOG_EXCEPTION( e );
            }
        }

        if( packet )
        {
            packet->resetReadPointer();
        }
    }

    void NetworkListener::connect( u32 playerId )
    {
        try
        {
            auto listeners = m_listeners.snapshot();
            for( auto &listener : listeners )
            {
                if( listener )
                    listener->connect( playerId );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void NetworkListener::disconnect( u32 playerId )
    {
        try
        {
            auto listeners = m_listeners.snapshot();
            for( auto &listener : listeners )
            {
                if( listener )
                    listener->disconnect( playerId );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    // -------------------------------------------------------------------------
    // Listener management
    // -------------------------------------------------------------------------

    void NetworkListener::addListener( SmartPtr<INetworkListener> listener )
    {
        if( !listener )
            return;

        if( !hasListener( listener ) )
            m_listeners.push_back( listener );
    }

    void NetworkListener::removeListener( SmartPtr<INetworkListener> listener )
    {
        if( !listener )
            return;

        // erase(const T&) removes all matching elements by value.
        m_listeners.erase( listener );
    }

    void NetworkListener::clearListeners()
    {
        m_listeners.clear();
    }

    bool NetworkListener::hasListener( SmartPtr<INetworkListener> listener ) const
    {
        const size_t idx = m_listeners.find_if(
            [&listener]( const SmartPtr<INetworkListener> &entry ) { return entry == listener; } );
        return idx != static_cast<size_t>( -1 );
    }

    u32 NetworkListener::getListenerCount() const
    {
        return static_cast<u32>( m_listeners.size() );
    }

}  // namespace workphone::scene
