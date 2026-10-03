#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CollisionTerrain.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/ITerrainShape.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, CollisionTerrain, Collision );

    const String CollisionTerrain::terrainWidthStr = "terrainWidth";
    const String CollisionTerrain::terrainDepthStr = "terrainDepth";
    const String CollisionTerrain::terrainScaleStr = "terrainScale";

    CollisionTerrain::CollisionTerrain() = default;

    CollisionTerrain::~CollisionTerrain()
    {
        if( isLoaded() )
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
            if( getLoadingState() != LoadingState::Loaded )
                return;

            setLoadingState( LoadingState::Unloading );
            Collision::unload( data );
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
        Collision::setProperties( properties );

        u32 width = m_terrainWidth;
        u32 depth = m_terrainDepth;
        Vector3<real_Num> scale = m_terrainScale;

        properties->getPropertyValue( terrainWidthStr, width );
        properties->getPropertyValue( terrainDepthStr, depth );
        properties->getPropertyValue( terrainScaleStr, scale );

        if( width != m_terrainWidth )
            setTerrainWidth( width );

        if( depth != m_terrainDepth )
            setTerrainDepth( depth );

        if( !MathUtil<real_Num>::equals( scale, m_terrainScale ) )
            setTerrainScale( scale );

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
                    return false;

                if( auto shape = getShape() )
                    return workphone::dynamic_pointer_cast<physics::ITerrainShape>( shape ) != nullptr;

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
        Collision::updateTransform();
    }

    void CollisionTerrain::createPhysicsShape()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "CollisionTerrain::createPhysicsShape - physics manager is not available" );
            return;
        }

        auto shape = physicsManager->addCollisionShape<physics::ITerrainShape>( nullptr );
        WP_ASSERT( shape );

        if( !shape )
            return;

        // Apply all stored base-class and local state to the new shape.
        shape->setTrigger( isTrigger() );
        shape->setEnabled( isEnabled() );

        Transform3<real_Num> pose;
        pose.setPosition( getPosition() );
        shape->setLocalPose( pose );

        if( auto mat = getMaterial() )
        {
            shape->setMaterial( mat );
            mat->setStaticFriction( getStaticFriction(), 0 );
            mat->setDynamicFriction( getDynamicFriction(), 0 );
            mat->setRestitution( getRestitution() );
        }

        setShape( shape );
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
                    createPhysicsShape();

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
