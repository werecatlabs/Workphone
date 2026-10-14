#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CollisionTerrain.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Physics/TerrainShape.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/ITerrainShape.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <algorithm>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, CollisionTerrain, Collision );

    const String CollisionTerrain::terrainWidthStr = "terrainWidth";
    const String CollisionTerrain::terrainDepthStr = "terrainDepth";
    const String CollisionTerrain::terrainScaleStr = "terrainScale";

    CollisionTerrain::CollisionTerrain() = default;

    CollisionTerrain::~CollisionTerrain()
    {
        discardPreparedTerrain();
        if( isLoaded() || getShape() )
        {
            unload( nullptr );
        }
    }

    void CollisionTerrain::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            Collision::load( data );

            createPhysicsShape();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CollisionTerrain::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() != LoadingState::Loaded && !getShape() && !m_preparedShape )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );
            discardPreparedTerrain();
            Collision::unload( data );
            m_terrainSnapshot.reset();
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> CollisionTerrain::getProperties() const
    {
        auto properties = Collision::getProperties();
        if( properties )
        {
            properties->setProperty( terrainWidthStr, m_terrainWidth );
            properties->setProperty( terrainDepthStr, m_terrainDepth );
            properties->setProperty( terrainScaleStr, m_terrainScale );
        }
        return properties;
    }

    void CollisionTerrain::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }
        Collision::setProperties( properties );

        u32 width = m_terrainWidth;
        u32 depth = m_terrainDepth;
        Vector3<real_Num> scale = m_terrainScale;

        properties->getPropertyValue( terrainWidthStr, width );
        properties->getPropertyValue( terrainDepthStr, depth );
        properties->getPropertyValue( terrainScaleStr, scale );

        if( width != m_terrainWidth )
        {
            setTerrainWidth( width );
        }

        if( depth != m_terrainDepth )
        {
            setTerrainDepth( depth );
        }

        if( !MathUtil<real_Num>::equals( scale, m_terrainScale ) )
        {
            setTerrainScale( scale );
        }

        updateTransform();
    }

    bool CollisionTerrain::isValid() const
    {
        if( auto actor = getActor() )
        {
            switch( actor->getState() )
            {
            case IGameActor::State::Edit:
            case IGameActor::State::Play:
            {
                if( !Collision::isValid() )
                {
                    return false;
                }

                if( auto shape = getShape() )
                {
                    return workphone::dynamic_pointer_cast<physics::ITerrainShape>( shape ) != nullptr;
                }

                return false;
            }
            default:
                return true;
            }
        }

        return false;
    }

    void CollisionTerrain::updateTransform()
    {
        if( auto actor = getActor() )
        {
            if( auto source = actor->getComponent<TerrainSystem>() )
            {
                source->updateTransform();
            }
        }
    }

    void CollisionTerrain::createPhysicsShape()
    {
        updateTransform();
        if( !getShape() && !m_preparedShape )
        {
            if( auto actor = getActor() )
            {
                if( auto source = actor->getComponent<TerrainSystem>() )
                {
                    if( auto terrain = dynamic_cast<render::Terrain *>( source->getTerrain().get() ) )
                    {
                        if( terrain->isTerrainPublicationDeferred() )
                        {
                            return;
                        }
                    }
                }
            }
            if( m_terrainError.empty() )
            {
                m_terrainError =
                    "A TerrainSystem and an available terrain physics backend are required.";
            }
            WP_LOG_WARNING( "CollisionTerrain: " + m_terrainError );
        }
    }

    bool CollisionTerrain::prepareTerrain( render::TerrainSnapshot snapshot,
                                           const Transform3<real_Num> &worldTransform, String &error )
    {
        const auto fail = [&]( const String &message ) {
            error = message;
            m_terrainError = message;
            return false;
        };
        if( !snapshot )
        {
            return fail( "Terrain collision has no authoritative source." );
        }
        if( !render::validateTerrainData( *snapshot, error ) ||
            !render::validateTerrainPlacement( *snapshot, worldTransform, error ) )
        {
            return fail( error );
        }
        const auto cells = size_t( snapshot->dimensions.x - 1 ) * ( snapshot->dimensions.y - 1 );
        const auto solidCells = snapshot->holes.empty()
                                    ? cells
                                    : static_cast<size_t>( std::count(
                                          snapshot->holes.begin(), snapshot->holes.end(), u8( 0 ) ) );
        if( solidCells * 2 > physics::TerrainCollisionData::maximumSupportedTriangles )
        {
            return fail(
                "Terrain collision exceeds 524288 solid triangles; tiled collision is unavailable." );
        }
        // Offsets would disagree with rendering and queries. Authoritative terrain
        // placement belongs exclusively to its actor, not legacy collision metadata.
        if( getPosition() != Vector3<real_Num>::zero() )
        {
            return fail( "Authoritative terrain collision requires a zero center offset." );
        }
        if( isTerrainPrepared( snapshot, worldTransform ) )
        {
            error.clear();
            m_terrainError.clear();
            return true;
        }
        const auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getPhysicsManager() : nullptr;
        if( !manager )
        {
            return fail( "Terrain collision physics manager is unavailable." );
        }
        discardPreparedTerrain();
        try
        {
            if( auto actor = getActor() )
            {
                setRigidBody( actor->getComponent<Rigidbody>() );
            }
            auto source = make_ptr<physics::TerrainCollisionData>();
            source->terrain = snapshot;
            source->scale = worldTransform.getScale();
            auto types = TypeManager::instance();
            const auto shapeType = types ? types->getHash( physics::ITerrainShape::typeInfo() ) : 0;
            if( !shapeType )
            {
                return fail( "Terrain collision type registration is unavailable." );
            }
            // Resource admission and native allocation may fail normally while
            // staging a replacement. The typed convenience factory asserts that
            // creation succeeded, so use the nullable factory at this boundary.
            auto shape = manager->addCollisionShapeByType( shapeType, source );
            auto concrete = dynamic_cast<physics::TerrainShape *>( shape.get() );
            if( !concrete || concrete->getTerrainSnapshot() != snapshot || !shape->isValid() )
            {
                if( shape )
                {
                    manager->removeCollisionShape( shape );
                }
                return fail( "Physics backend failed to prepare the authoritative terrain revision." );
            }
            m_preparedShape = shape;
            shape->setTrigger( isTrigger() );
            shape->setEnabled( isEnabled() );
            if( auto previous = getShape() )
            {
                shape->setSimulationFilterData( previous->getSimulationFilterData() );
            }
            if( auto material = getMaterial() )
            {
                shape->setMaterial( material );
                material->setStaticFriction( getStaticFriction(), 0 );
                material->setDynamicFriction( getDynamicFriction(), 0 );
                material->setRestitution( getRestitution() );
            }
            m_preparedSnapshot = snapshot;
            m_preparedTransform = worldTransform;
            m_terrainError.clear();
            error.clear();
            return true;
        }
        catch( const std::exception &exception )
        {
            discardPreparedTerrain();
            return fail( String( "Unable to prepare terrain collision: " ) + exception.what() );
        }
    }

    void CollisionTerrain::commitTerrain()
    {
        if( !m_preparedShape )
        {
            return;
        }
        auto previous = getShape();
        auto body =
            previous ? dynamic_pointer_cast<physics::IRigidBody3>( previous->getActor() ) : nullptr;
        if( !body )
        {
            if( auto component = getRigidBody() )
            {
                if( auto rigid = component->getRigidStatic() )
                {
                    body = rigid;
                }
                else if( auto rigid = component->getRigidDynamic() )
                {
                    body = rigid;
                }
            }
        }
        setShape( m_preparedShape );
        if( body )
        {
            if( previous )
            {
                body->removeShape( previous, false );
            }
            else
            {
                m_preparedShape->setCollisionType( body->getCollisionType() );
                m_preparedShape->setCollisionMask( body->getCollisionMask() );
            }
            body->addShape( m_preparedShape );
            auto pose = m_preparedTransform;
            pose.setScale( Vector3<real_Num>::unit() );
            body->setTransform( pose );
        }
        // Avoid Rigidbody::updateShapes: its actor transform callback re-enters the
        // TerrainSystem publication mutex. Existing bodies retain their scene registration.
        if( previous )
        {
            if( const auto app = core::IApplicationManager::instancePtr() )
            {
                if( auto manager = app->getPhysicsManager() )
                {
                    manager->removeCollisionShape( previous );
                }
            }
        }
        m_terrainSnapshot = m_preparedSnapshot;
        m_publishedTransform = m_preparedTransform;
        m_terrainWidth = static_cast<u32>( m_terrainSnapshot->dimensions.x );
        m_terrainDepth = static_cast<u32>( m_terrainSnapshot->dimensions.y );
        m_terrainScale = m_publishedTransform.getScale();
        m_preparedShape = nullptr;
        m_preparedSnapshot.reset();
    }

    void CollisionTerrain::discardPreparedTerrain()
    {
        if( m_preparedShape )
        {
            if( const auto app = core::IApplicationManager::instancePtr() )
            {
                if( auto manager = app->getPhysicsManager() )
                {
                    manager->removeCollisionShape( m_preparedShape );
                }
            }
        }
        m_preparedShape = nullptr;
        m_preparedSnapshot.reset();
    }

    render::TerrainSnapshot CollisionTerrain::getTerrainSnapshot() const
    {
        return m_terrainSnapshot;
    }

    u64 CollisionTerrain::getTerrainRevision() const
    {
        return m_terrainSnapshot ? m_terrainSnapshot->revision : 0;
    }

    const String &CollisionTerrain::getTerrainError() const
    {
        return m_terrainError;
    }

    Transform3<real_Num> CollisionTerrain::getPublishedTerrainTransform() const
    {
        return m_publishedTransform;
    }

    bool CollisionTerrain::isTerrainPrepared( render::TerrainSnapshot snapshot,
                                              const Transform3<real_Num> &placement ) const
    {
        return ( m_preparedShape && m_preparedSnapshot == snapshot &&
                 m_preparedTransform == placement ) ||
               ( getShape() && m_terrainSnapshot == snapshot && m_publishedTransform == placement );
    }

    FSMReturnType CollisionTerrain::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        Collision::handleComponentEvent( state, eventType );

        switch( eventType )
        {
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                if( !getShape() )
                {
                    createPhysicsShape();
                }

                updateTransform();
                updateRigidBody();
            }
            break;
            default:
                break;
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                // Shape lifetime is managed by unload; nothing to do here.
            }
            break;
            default:
                break;
            }
        }
        break;
        default:
            break;
        }

        return FSMReturnType::Ok;
    }

    // --- Accessors -----------------------------------------------------------

    u32 CollisionTerrain::getTerrainWidth() const
    {
        return m_terrainWidth;
    }

    void CollisionTerrain::setTerrainWidth( u32 width )
    {
        m_terrainWidth = width;
    }

    u32 CollisionTerrain::getTerrainDepth() const
    {
        return m_terrainDepth;
    }

    void CollisionTerrain::setTerrainDepth( u32 depth )
    {
        m_terrainDepth = depth;
    }

    Vector3<real_Num> CollisionTerrain::getTerrainScale() const
    {
        return m_terrainScale;
    }

    void CollisionTerrain::setTerrainScale( const Vector3<real_Num> &scale )
    {
        m_terrainScale = scale;
    }

}  // namespace workphone::scene
