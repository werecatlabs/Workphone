#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/NetworkView.hpp>
#include <Workphone/Interface/Net/INetworkManager.hpp>
#include <Workphone/Interface/Net/INetworkStream.hpp>
#include <Workphone/Interface/Net/IPacket.hpp>
#include <Workphone/Interface/Net/NetworkMessage.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>

#include <algorithm>
#include <cmath>

namespace workphone::scene
{
    namespace
    {
        constexpr u8 SyncPositionFlag = 1u << 0u;
        constexpr u8 SyncRotationFlag = 1u << 1u;
        constexpr u8 SyncScaleFlag = 1u << 2u;
    }  // namespace

    // -------------------------------------------------------------------------
    // NetworkView::Listener
    // -------------------------------------------------------------------------

    WP_CLASS_REGISTER_DERIVED( workphone::scene, NetworkView::Listener, INetworkListener );

    NetworkView::Listener::Listener( NetworkView *owner ) : m_owner( owner )
    {
    }

    NetworkView::Listener::~Listener() = default;

    void NetworkView::Listener::handlePacket( SmartPtr<IPacket> packet )
    {
        if( m_owner && packet )
            m_owner->handlePacket( packet );
    }

    void NetworkView::Listener::connect( u32 /*playerId*/ )
    {
    }

    void NetworkView::Listener::disconnect( u32 /*playerId*/ )
    {
    }

    // -------------------------------------------------------------------------
    // NetworkView
    // -------------------------------------------------------------------------

    WP_CLASS_REGISTER_DERIVED( workphone::scene, NetworkView, Component );

    const String NetworkView::viewIdStr = "viewId";
    const String NetworkView::ownerIdStr = "ownerId";
    const String NetworkView::sceneViewStr = "sceneView";
    const String NetworkView::replicationEnabledStr = "replicationEnabled";
    const String NetworkView::authorityModeStr = "authorityMode";
    const String NetworkView::deliveryModeStr = "deliveryMode";
    const String NetworkView::sendRateStr = "sendRate";
    const String NetworkView::syncPositionStr = "syncPosition";
    const String NetworkView::syncRotationStr = "syncRotation";
    const String NetworkView::syncScaleStr = "syncScale";
    const String NetworkView::allowOwnershipRequestsStr = "allowOwnershipRequests";

    const Array<String> NetworkView::authorityModeNames = { "Owner", "Server", "Any Peer" };
    const Array<String> NetworkView::deliveryModeNames = { "Unreliable", "Reliable" };

    NetworkView::NetworkView() = default;

    NetworkView::~NetworkView()
    {
        unload( nullptr );
    }

    void NetworkView::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            m_networkManager = workphone::static_pointer_cast<INetworkManager>(
                applicationManager->getNetworkManager() );

            if( m_networkManager )
            {
                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                m_listener = factoryManager->make_ptr<Listener>( this );
                m_networkManager->addListener( m_listener );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void NetworkView::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            if( m_networkManager )
            {
                m_networkManager->removeListener( m_listener );
                m_networkManager = nullptr;
            }

            m_listener = nullptr;

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void NetworkView::update()
    {
        if( !isEnabled() || !m_replicationEnabled || m_viewId < 0 || !m_networkManager ||
            !hasSendAuthority() ||
            m_networkManager->getConnectionStatus() !=
                INetworkManager::ConnectionStatus::NCS_ESTABLISHED )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager ? applicationManager->getTimerPtr() : nullptr;
        const auto deltaTime =
            timer ? std::max( 0.0f, static_cast<f32>( timer->getDeltaTime() ) ) : 0.0f;
        const auto interval = 1.0f / std::max( 1.0f, m_sendRate );
        m_sendAccumulator += deltaTime;

        if( m_sendAccumulator >= interval )
        {
            m_sendAccumulator = std::fmod( m_sendAccumulator, interval );
            serializeView();
        }
    }

    SmartPtr<Properties> NetworkView::getProperties() const
    {
        auto properties = Component::getProperties();
        properties->setProperty( viewIdStr, m_viewId );
        properties->setProperty( ownerIdStr, m_ownerId );
        properties->setProperty( sceneViewStr, m_sceneView );
        properties->setProperty( replicationEnabledStr, m_replicationEnabled );
        properties->setPropertyAsEnum( authorityModeStr, static_cast<s32>( m_authorityMode ),
                                       authorityModeNames );
        properties->setPropertyAsEnum( deliveryModeStr, static_cast<s32>( m_deliveryMode ),
                                       deliveryModeNames );
        properties->setProperty( sendRateStr, m_sendRate );
        properties->setProperty( syncPositionStr, m_syncPosition );
        properties->setProperty( syncRotationStr, m_syncRotation );
        properties->setProperty( syncScaleStr, m_syncScale );
        properties->setProperty( allowOwnershipRequestsStr, m_allowOwnershipRequests );

        const auto setEditorMetadata = [&]( const String &name, const String &label,
                                            const String &category, const String &description ) {
            if( properties->hasProperty( name ) )
            {
                auto &property = properties->getPropertyObject( name );
                property.setAttribute( "label", label );
                property.setAttribute( "category", category );
                property.setAttribute( "description", description );
            }
        };

        setEditorMetadata( viewIdStr, "View ID", "Network Identity",
                           "Session-unique identifier used to route actor messages." );
        setEditorMetadata( ownerIdStr, "Owner ID", "Network Identity",
                           "Player identifier that owns this actor when authority is Owner." );
        setEditorMetadata( sceneViewStr, "Scene View", "Network Identity",
                           "Marks a design-time scene actor rather than a player-spawned actor." );
        setEditorMetadata( replicationEnabledStr, "Replicate", "Replication",
                           "Enables automatic actor state snapshots." );
        setEditorMetadata( authorityModeStr, "Authority", "Replication",
                           "Selects which peer is allowed to publish actor state." );
        setEditorMetadata(
            deliveryModeStr, "Delivery", "Replication",
            "Unreliable is preferred for frequent snapshots; Reliable guarantees delivery." );
        setEditorMetadata( sendRateStr, "Send Rate (Hz)", "Replication",
                           "Maximum number of actor snapshots sent per second." );
        setEditorMetadata( syncPositionStr, "Position", "Transform Channels",
                           "Include world position in actor snapshots." );
        setEditorMetadata( syncRotationStr, "Rotation", "Transform Channels",
                           "Include world Euler rotation in actor snapshots." );
        setEditorMetadata( syncScaleStr, "Scale", "Transform Channels",
                           "Include world scale in actor snapshots." );
        setEditorMetadata( allowOwnershipRequestsStr, "Auto-accept Requests", "Ownership",
                           "Allow the current authority to grant ownership requests automatically." );

        auto &viewIdProperty = properties->getPropertyObject( viewIdStr );
        viewIdProperty.setAttribute( "min", "-1" );
        viewIdProperty.setAttribute( "step", "1" );

        auto &ownerIdProperty = properties->getPropertyObject( ownerIdStr );
        ownerIdProperty.setAttribute( "min", "0" );
        ownerIdProperty.setAttribute( "step", "1" );

        auto &sendRateProperty = properties->getPropertyObject( sendRateStr );
        sendRateProperty.setAttribute( "min", "1" );
        sendRateProperty.setAttribute( "max", "120" );
        sendRateProperty.setAttribute( "step", "1" );

        return properties;
    }

    void NetworkView::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        if( !properties )
        {
            return;
        }

        properties->getPropertyValue( viewIdStr, m_viewId );
        properties->getPropertyValue( ownerIdStr, m_ownerId );
        properties->getPropertyValue( sceneViewStr, m_sceneView );
        properties->getPropertyValue( replicationEnabledStr, m_replicationEnabled );

        auto authorityMode = static_cast<s32>( m_authorityMode );
        properties->getPropertyValue( authorityModeStr, authorityMode );
        authorityMode =
            std::max( 0, std::min( authorityMode, static_cast<s32>( AuthorityMode::Count ) - 1 ) );
        m_authorityMode = static_cast<AuthorityMode>( authorityMode );

        auto deliveryMode = static_cast<s32>( m_deliveryMode );
        properties->getPropertyValue( deliveryModeStr, deliveryMode );
        deliveryMode =
            std::max( 0, std::min( deliveryMode, static_cast<s32>( DeliveryMode::Count ) - 1 ) );
        m_deliveryMode = static_cast<DeliveryMode>( deliveryMode );

        properties->getPropertyValue( sendRateStr, m_sendRate );
        m_sendRate = std::max( 1.0f, std::min( m_sendRate, 120.0f ) );
        properties->getPropertyValue( syncPositionStr, m_syncPosition );
        properties->getPropertyValue( syncRotationStr, m_syncRotation );
        properties->getPropertyValue( syncScaleStr, m_syncScale );
        properties->getPropertyValue( allowOwnershipRequestsStr, m_allowOwnershipRequests );
    }

    void NetworkView::RPC( SmartPtr<IPacket> packet )
    {
        if( !m_networkManager || !packet )
            return;

        m_networkManager->sendPacket( packet );
    }

    void NetworkView::serializeView()
    {
        if( !m_networkManager || !m_replicationEnabled || m_viewId < 0 || !hasSendAuthority() )
            return;

        auto actor = getActor();
        if( !actor )
            return;

        auto transform = actor->getTransform();
        if( !transform )
            return;

        auto packet = m_networkManager->createPacket();
        if( !packet )
            return;

        NetworkMessageHeader header;
        header.type = NetworkMessageType::ActorState;
        header.objectId = m_viewId;
        header.write( packet );

        packet->write( ++m_outgoingSequence );

        u8 channels = 0;
        channels |= m_syncPosition ? SyncPositionFlag : 0;
        channels |= m_syncRotation ? SyncRotationFlag : 0;
        channels |= m_syncScale ? SyncScaleFlag : 0;
        packet->write( channels );

        if( m_syncPosition )
        {
            packet->write( transform->getPosition() );
        }
        if( m_syncRotation )
        {
            packet->write( transform->getRotation() );
        }
        if( m_syncScale )
        {
            packet->write( transform->getScale() );
        }

        if( m_deliveryMode == DeliveryMode::Reliable )
        {
            m_networkManager->sendPacket( packet );
        }
        else
        {
            m_networkManager->sendPacketUnreliable( packet );
        }
    }

    void NetworkView::deserializeView( SmartPtr<IPacket> packet )
    {
        if( !packet )
            return;

        auto actor = getActor();
        if( !actor )
            return;

        auto transform = actor->getTransform();
        if( !transform )
            return;

        u32 sequence = 0;
        u8 channels = 0;
        packet->read( sequence );
        packet->read( channels );

        if( m_hasReceivedSnapshot && static_cast<s32>( sequence - m_lastReceivedSequence ) <= 0 )
        {
            return;
        }

        if( ( channels & SyncPositionFlag ) != 0 )
        {
            auto position = Vector3<real_Num>();
            packet->read( position );
            transform->setPosition( position );
        }
        if( ( channels & SyncRotationFlag ) != 0 )
        {
            auto rotation = Vector3<real_Num>();
            packet->read( rotation );
            transform->setRotation( rotation );
        }
        if( ( channels & SyncScaleFlag ) != 0 )
        {
            auto scale = Vector3<real_Num>();
            packet->read( scale );
            transform->setScale( scale );
        }

        m_lastReceivedSequence = sequence;
        m_hasReceivedSnapshot = true;
    }

    void NetworkView::handlePacket( SmartPtr<IPacket> packet )
    {
        if( !packet )
            return;

        NetworkMessageHeader header;
        if( !NetworkMessageHeader::read( packet, header ) || header.objectId != m_viewId )
            return;

        switch( header.type )
        {
        case NetworkMessageType::ActorState:
            deserializeView( packet );
            break;
        case NetworkMessageType::OwnershipTransfer:
        {
            u32 newOwnerId = 0;
            packet->read( newOwnerId );
            setOwnerId( newOwnerId );
        }
        break;
        case NetworkMessageType::OwnershipRequest:
        {
            u32 requestingPlayerId = 0;
            packet->read( requestingPlayerId );
            if( m_allowOwnershipRequests && hasSendAuthority() )
            {
                transferOwnership( requestingPlayerId );
            }
        }
        break;
        default:
            break;
        }
    }

    s32 NetworkView::getViewId() const
    {
        return m_viewId;
    }

    void NetworkView::setViewId( s32 viewId )
    {
        m_viewId = viewId;
    }

    u32 NetworkView::getOwnerId() const
    {
        return m_ownerId;
    }

    void NetworkView::setOwnerId( u32 ownerId )
    {
        m_ownerId = ownerId;
    }

    bool NetworkView::isMine() const
    {
        if( !m_networkManager )
            return false;

        if( m_networkManager->isServer() )
        {
            return m_ownerId == 0;
        }

        return static_cast<u32>( m_networkManager->getPlayerNumber() ) == m_ownerId;
    }

    bool NetworkView::hasSendAuthority() const
    {
        if( !m_networkManager )
        {
            return false;
        }

        switch( m_authorityMode )
        {
        case AuthorityMode::Owner:
            return isMine();
        case AuthorityMode::Server:
            return m_networkManager->isServer();
        case AuthorityMode::AnyPeer:
            return true;
        default:
            return false;
        }
    }

    void NetworkView::onSerializeView( SmartPtr<INetworkStream> stream )
    {
        // Default implementation is a no-op.
        // Subclasses or user-code components should override / delegate here
        // to push or pull custom per-actor state through the shared stream,
        // exactly as OnPhotonSerializeView works in Unity PUN.
        (void)stream;
    }

    void NetworkView::transferOwnership( u32 newOwnerId )
    {
        if( !m_networkManager )
            return;

        auto packet = m_networkManager->createPacket();
        if( !packet )
            return;

        NetworkMessageHeader header;
        header.type = NetworkMessageType::OwnershipTransfer;
        header.objectId = m_viewId;
        header.write( packet );
        packet->write( newOwnerId );

        m_ownerId = newOwnerId;
        m_networkManager->sendPacket( packet );
    }

    void NetworkView::requestOwnership()
    {
        if( !m_networkManager )
            return;

        auto packet = m_networkManager->createPacket();
        if( !packet )
            return;

        NetworkMessageHeader header;
        header.type = NetworkMessageType::OwnershipRequest;
        header.objectId = m_viewId;
        header.write( packet );

        const u32 localPlayer =
            m_networkManager->isServer() ? 0u : static_cast<u32>( m_networkManager->getPlayerNumber() );
        packet->write( localPlayer );

        m_networkManager->sendPacket( packet );
    }

    bool NetworkView::isSceneView() const
    {
        return m_sceneView;
    }

    bool NetworkView::isReplicationEnabled() const
    {
        return m_replicationEnabled;
    }

    void NetworkView::setReplicationEnabled( bool enabled )
    {
        m_replicationEnabled = enabled;
        if( !enabled )
        {
            m_sendAccumulator = 0.0f;
        }
    }

    NetworkView::AuthorityMode NetworkView::getAuthorityMode() const
    {
        return m_authorityMode;
    }

    void NetworkView::setAuthorityMode( AuthorityMode authorityMode )
    {
        const auto value = static_cast<s32>( authorityMode );
        m_authorityMode = value >= 0 && value < static_cast<s32>( AuthorityMode::Count )
                              ? authorityMode
                              : AuthorityMode::Server;
    }

    NetworkView::DeliveryMode NetworkView::getDeliveryMode() const
    {
        return m_deliveryMode;
    }

    void NetworkView::setDeliveryMode( DeliveryMode deliveryMode )
    {
        const auto value = static_cast<s32>( deliveryMode );
        m_deliveryMode = value >= 0 && value < static_cast<s32>( DeliveryMode::Count )
                             ? deliveryMode
                             : DeliveryMode::Unreliable;
    }

    f32 NetworkView::getSendRate() const
    {
        return m_sendRate;
    }

    void NetworkView::setSendRate( f32 sendRate )
    {
        m_sendRate = std::max( 1.0f, std::min( sendRate, 120.0f ) );
    }

    bool NetworkView::getSyncPosition() const
    {
        return m_syncPosition;
    }

    void NetworkView::setSyncPosition( bool syncPosition )
    {
        m_syncPosition = syncPosition;
    }

    bool NetworkView::getSyncRotation() const
    {
        return m_syncRotation;
    }

    void NetworkView::setSyncRotation( bool syncRotation )
    {
        m_syncRotation = syncRotation;
    }

    bool NetworkView::getSyncScale() const
    {
        return m_syncScale;
    }

    void NetworkView::setSyncScale( bool syncScale )
    {
        m_syncScale = syncScale;
    }

    bool NetworkView::getAllowOwnershipRequests() const
    {
        return m_allowOwnershipRequests;
    }

    void NetworkView::setAllowOwnershipRequests( bool allowOwnershipRequests )
    {
        m_allowOwnershipRequests = allowOwnershipRequests;
    }

    void NetworkView::CNetworkView::setOwner( NetworkView *owner )
    {
        m_owner = owner;
    }

    NetworkView *NetworkView::CNetworkView::getOwner() const
    {
        return m_owner;
    }

    bool NetworkView::CNetworkView::isSceneView() const
    {
        return m_owner->isSceneView();
    }

    void NetworkView::CNetworkView::requestOwnership()
    {
        m_owner->requestOwnership();
    }

    void NetworkView::CNetworkView::transferOwnership( u32 newOwnerId )
    {
        m_owner->transferOwnership( newOwnerId );
    }

    bool NetworkView::CNetworkView::isMine() const
    {
        return m_owner->isMine();
    }

    void NetworkView::CNetworkView::setOwnerId( u32 ownerId )
    {
        m_owner->setOwnerId( ownerId );
    }

    u32 NetworkView::CNetworkView::getOwnerId() const
    {
        return m_owner->getOwnerId();
    }

    void NetworkView::CNetworkView::setViewId( s32 viewId )
    {
        m_owner->setViewId( viewId );
    }

    s32 NetworkView::CNetworkView::getViewId() const
    {
        return m_owner->getViewId();
    }

    void NetworkView::CNetworkView::onSerializeView( SmartPtr<INetworkStream> stream )
    {
        m_owner->onSerializeView( stream );
    }

    void NetworkView::CNetworkView::deserializeView( SmartPtr<IPacket> packet )
    {
        m_owner->deserializeView( packet );
    }

    void NetworkView::CNetworkView::serializeView()
    {
        m_owner->serializeView();
    }

    void NetworkView::CNetworkView::RPC( SmartPtr<IPacket> packet )
    {
        if( m_owner )
        {
            m_owner->RPC( packet );
        }
    }

    NetworkView::CNetworkView::~CNetworkView()
    {
    }

    NetworkView::CNetworkView::CNetworkView( NetworkView *owner ) : m_owner( owner )
    {
    }

    NetworkView::CNetworkView::CNetworkView() = default;

}  // namespace workphone::scene
