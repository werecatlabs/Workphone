#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Math/Euler.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    const hash_type Transform::TRANSFORMATION_POSITION_HASH =
        StringUtil::getHash( "TransformationPosition" );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Transform, ITransform );

    Transform::Transform()
    {
        m_handle = TransformSystem::instance().createTransform();
        TransformSystem::instance().setFlags( m_handle, enabledFlag );
    }

    Transform::~Transform()
    {
        TransformSystem::instance().destroyTransform( m_handle );
    }

    void Transform::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            auto factoryManager = applicationManager->getFactoryManager();

            WP_ASSERT( stateManager );
            WP_ASSERT( factoryManager );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Transform::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            auto sceneManager =
                workphone::static_pointer_cast<GameManager>( applicationManager->getGameManager() );

            setActor( nullptr );

            auto flags = getFlags();
            if( BitUtil::getFlagValue( flags, smoothMotionFlag ) )
            {
                if( sceneManager )
                {
                    sceneManager->removeSmoothTransform( this );
                }
            }

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Transform::update()
    {
        if( isLocalDirty() )
        {
            updateLocalFromWorld();
        }

        if( isDirty() )
        {
            updateWorldFromLocal();
        }
    }

    void Transform::updateLocalFromWorld()
    {
        Transform3<real_Num> parentWorldTransform;
        const Transform3<real_Num> *parentWorldTransformPtr = nullptr;
        auto canUpdateTransform = true;

        if( auto actor = getActorPtr() )
        {
            if( auto parent = actor->getParentPtr() )
            {
                if( auto parentTransform = parent->getTransformPtr() )
                {
                    parentWorldTransform = parentTransform->getWorldTransform();
                    parentWorldTransformPtr = &parentWorldTransform;
                }
                else
                {
                    canUpdateTransform = false;
                }
            }
        }

        auto &transformSystem = TransformSystem::instance();
        if( canUpdateTransform )
        {
            transformSystem.updateLocalFromWorld( m_handle, parentWorldTransformPtr );
        }
        transformSystem.setFlag( m_handle, localDirtyFlag, false );
    }

    void Transform::parentChanged( SmartPtr<IGameActor> newParent, SmartPtr<IGameActor> oldParent )
    {
        setDirty( true );
    }

    IGameActor *Transform::getActorPtr() const
    {
        return TransformSystem::instance().getActorPtr( m_handle );
    }

    SmartPtr<IGameActor> Transform::getActor() const
    {
        return TransformSystem::instance().getActor( m_handle );
    }

    void Transform::setActor( SmartPtr<IGameActor> actor )
    {
        TransformSystem::instance().setActor( m_handle, actor );
    }

    bool Transform::isLocalDirty() const
    {
        return TransformSystem::instance().getFlag( m_handle, localDirtyFlag );
    }

    void Transform::setLocalDirty( bool localDirty, bool cascade )
    {
        auto updateTransform = false;
        auto dirty = isLocalDirty();
        if( dirty != localDirty )
        {
            TransformSystem::instance().setFlag( m_handle, localDirtyFlag, localDirty );
            updateTransform = dirty == true || localDirty == true;
        }

        if( updateTransform )
        {
            if( localDirty )
            {
                updateFrameTime();

                if( auto actor = getActorPtr() )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );

                    auto gameManager = applicationManager->getGameManagerPtr();

                    auto transform = getSharedFromThis<ITransform>();
                    gameManager->addDirtyTransform( transform );

                    auto components = actor->getComponents();
                    for( auto &component : components )
                    {
                        gameManager->addDirtyComponentTransform( component );
                    }

                    if( cascade )
                    {
                        auto children = actor->getChildren();
                        for( auto &child : children )
                        {
                            WP_ASSERT( child );

                            auto childTransform = child->getTransform();
                            WP_ASSERT( childTransform );

                            // The parent's world pose changed; children keep their local pose and
                            // rebuild their world pose. Rebuilding their local pose instead keeps
                            // them at the previous world position and makes animated wheels orbit.
                            childTransform->setDirty( true, cascade );
                        }
                    }
                }
            }
        }
    }

    bool Transform::isDirty() const
    {
        return TransformSystem::instance().getFlag( m_handle, dirtyFlag );
    }

    void Transform::setDirty( bool dirty, bool cascade )
    {
        auto updateTransform = false;
        auto dirtyValue = isDirty();
        if( dirtyValue != dirty )
        {
            TransformSystem::instance().setFlag( m_handle, dirtyFlag, dirty );
            updateTransform = dirty == true || dirtyValue == true;
        }

        if( updateTransform )
        {
            if( dirty )
            {
                updateFrameTime();

                if( auto actor = getActor() )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );

                    auto sceneManager = applicationManager->getGameManager();

                    auto transform = getSharedFromThis<ITransform>();
                    sceneManager->addDirtyTransform( transform );

                    auto components = actor->getComponents();
                    for( auto &component : components )
                    {
                        sceneManager->addDirtyComponentTransform( component );
                    }

                    if( cascade )
                    {
                        auto children = actor->getChildren();
                        for( auto &child : children )
                        {
                            if( child )
                            {
                                if( auto childTransform = child->getTransform() )
                                {
                                    childTransform->setDirty( true, cascade );
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void Transform::setEnabled( bool enabled )
    {
        TransformSystem::instance().setFlag( m_handle, enabledFlag, enabled );
    }

    bool Transform::isEnabled() const
    {
        return TransformSystem::instance().getFlag( m_handle, enabledFlag );
    }

    Transform3<real_Num> Transform::getLocalTransform() const
    {
        return TransformSystem::instance().getLocalTransform( m_handle );
    }

    void Transform::setLocalTransform( Transform3<real_Num> transform )
    {
        if( getTransformReferences() > 0 )
        {
            return;
        }

        TransformSystem::instance().setLocalTransform( m_handle, transform );
    }

    Transform3<real_Num> Transform::getWorldTransform() const
    {
        return TransformSystem::instance().getWorldTransform( m_handle );
    }

    void Transform::setWorldTransform( Transform3<real_Num> transform )
    {
        if( getTransformReferences() > 0 )
        {
            return;
        }

        TransformSystem::instance().setWorldTransform( m_handle, transform );
    }

    Vector3<real_Num> Transform::getLocalPosition() const
    {
        return TransformSystem::instance().getLocalPosition( m_handle );
    }

    void Transform::setLocalPosition( const Vector3<real_Num> &localPosition )
    {
        if( getTransformReferences() > 0 )
        {
            return;
        }

        TransformSystem::instance().setLocalPosition( m_handle, localPosition );
    }

    Vector3<real_Num> Transform::getLocalScale() const
    {
        return TransformSystem::instance().getLocalScale( m_handle );
    }

    void Transform::setLocalScale( const Vector3<real_Num> &localScale )
    {
        if( getTransformReferences() > 0 )
        {
            return;
        }

        TransformSystem::instance().setLocalScale( m_handle, localScale );
    }

    Quaternion<real_Num> Transform::getLocalOrientation() const
    {
        return TransformSystem::instance().getLocalOrientation( m_handle );
    }

    void Transform::setLocalOrientation( const Quaternion<real_Num> &localOrientation )
    {
        if( getTransformReferences() > 0 )
        {
            return;
        }

        TransformSystem::instance().setLocalOrientation( m_handle, localOrientation );
    }

    Vector3<real_Num> Transform::getLocalRotation() const
    {
        return getLocalTransform().getRotation();
    }

    void Transform::setLocalRotation( const Vector3<real_Num> &localRotation )
    {
        auto localTransform = getLocalTransform();
        localTransform.setRotation( localRotation );
        setLocalTransform( localTransform );
    }

    Vector3<real_Num> Transform::getPosition() const
    {
        return TransformSystem::instance().getWorldPosition( m_handle );
    }

    void Transform::setPosition( const Vector3<real_Num> &position )
    {
        if( getTransformReferences() > 0 )
        {
            return;
        }

        TransformSystem::instance().setWorldPosition( m_handle, position );
    }

    Vector3<real_Num> Transform::getScale() const
    {
        return TransformSystem::instance().getWorldScale( m_handle );
    }

    void Transform::setScale( const Vector3<real_Num> &scale )
    {
        if( getTransformReferences() > 0 )
        {
            return;
        }

        TransformSystem::instance().setWorldScale( m_handle, scale );
    }

    Quaternion<real_Num> Transform::getOrientation() const
    {
        return TransformSystem::instance().getWorldOrientation( m_handle );
    }

    void Transform::setOrientation( const Quaternion<real_Num> &orientation )
    {
        if( getTransformReferences() > 0 )
        {
            return;
        }

        TransformSystem::instance().setWorldOrientation( m_handle, orientation );
    }

    Vector3<real_Num> Transform::getRotation() const
    {
        return getWorldTransform().getRotation();
    }

    void Transform::setRotation( const Vector3<real_Num> &rotation )
    {
        auto worldTransform = getWorldTransform();
        worldTransform.setRotation( rotation );
        setWorldTransform( worldTransform );
    }

    void Transform::updateWorldFromLocal()
    {
        if( isLoaded() )
        {
            Transform3<real_Num> parentWorldTransform;
            const Transform3<real_Num> *parentWorldTransformPtr = nullptr;
            auto canUpdateTransform = true;

            if( auto actor = getActorPtr() )
            {
                auto parent = actor->getParentPtr();
                if( parent )
                {
                    if( auto parentTransform = parent->getTransformPtr() )
                    {
                        parentWorldTransform = parentTransform->getWorldTransform();
                        parentWorldTransformPtr = &parentWorldTransform;
                    }
                    else
                    {
                        canUpdateTransform = false;
                    }
                }
            }

            auto &transformSystem = TransformSystem::instance();
            if( canUpdateTransform )
            {
                transformSystem.updateWorldFromLocal( m_handle, parentWorldTransformPtr );
            }
            transformSystem.setFlag( m_handle, dirtyFlag, false );
        }
    }

    SmartPtr<Properties> Transform::getProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();

        Transform3<real_Num> localTransform;
        Transform3<real_Num> worldTransform;
        TransformSystem::instance().getTransforms( m_handle, localTransform, worldTransform );

        auto localPosition = localTransform.getPosition();
        auto localOrientation = localTransform.getOrientation();
        auto localScale = localTransform.getScale();

        auto position = worldTransform.getPosition();
        auto orientation = worldTransform.getOrientation();
        auto scale = worldTransform.getScale();

        Euler<real_Num> eular( localOrientation );
        Vector3<real_Num> localRotation = eular.toDegrees();

        Euler<real_Num> worldEular( orientation );
        Vector3<real_Num> rotation = worldEular.toDegrees();

        properties->setProperty( "Local Position", localPosition );
        properties->setProperty( "Local Rotation", localRotation );
        properties->setProperty( "Local Scale", localScale );

        properties->setProperty( "Position", position );
        properties->setProperty( "Rotation", rotation );
        properties->setProperty( "Scale", scale );

        return properties;
    }

    void Transform::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            Transform3<real_Num> localTransform;
            Transform3<real_Num> worldTransform;
            TransformSystem::instance().getTransforms( m_handle, localTransform, worldTransform );

            auto currentLocalPosition = localTransform.getPosition();
            auto currentLocalOrientation = localTransform.getOrientation();
            auto currentLocalScale = localTransform.getScale();

            auto currentPosition = worldTransform.getPosition();
            auto currentOrientation = worldTransform.getOrientation();
            auto currentScale = worldTransform.getScale();

            auto currentLocalRotation = Vector3<real_Num>::zero();
            auto currentRotation = Vector3<real_Num>::zero();

            currentLocalOrientation.fromDegrees( currentLocalRotation );
            currentOrientation.fromDegrees( currentRotation );

            currentLocalRotation = MathUtil<real_Num>::round( currentLocalRotation, 3 );
            currentRotation = MathUtil<real_Num>::round( currentRotation, 3 );

            auto localPosition = Vector3<real_Num>::zero();
            auto localOrientation = Quaternion<real_Num>::identity();
            auto localScale = Vector3<real_Num>::unit();

            auto position = Vector3<real_Num>::zero();
            auto orientation = Quaternion<real_Num>::identity();
            auto scale = Vector3<real_Num>::unit();

            auto localRotation = Vector3<real_Num>::zero();
            auto rotation = Vector3<real_Num>::zero();

            properties->getPropertyValue( "Local Position", localPosition );
            properties->getPropertyValue( "Local Rotation", localRotation );
            properties->getPropertyValue( "Local Scale", localScale );

            properties->getPropertyValue( "Position", position );
            properties->getPropertyValue( "Rotation", rotation );
            properties->getPropertyValue( "Scale", scale );

            localRotation = MathUtil<real_Num>::round( localRotation, 3 );
            rotation = MathUtil<real_Num>::round( rotation, 3 );

            localOrientation.fromDegrees( localRotation );
            orientation.fromDegrees( rotation );

            auto propertiesArray = properties->getPropertiesAsArray();
            for( auto &p : propertiesArray )
            {
                auto attibute = p.getAttribute( "changed" );
                if( attibute == "true" )
                {
                    auto propertyName = p.getName();
                    if( propertyName == "Local Position" || propertyName == "Local Rotation" ||
                        propertyName == "Local Scale" )
                    {
                        setDirty( true );
                    }

                    if( propertyName == "Position" || propertyName == "Rotation" ||
                        propertyName == "Scale" )
                    {
                        setLocalDirty( true );
                    }
                }
            }

            if( currentLocalPosition != localPosition )
            {
                localTransform.setPosition( localPosition );
            }

            if( currentLocalOrientation != localOrientation )
            {
                localTransform.setOrientation( localOrientation );
            }

            if( currentLocalScale != localScale )
            {
                localTransform.setScale( localScale );
            }

            if( currentPosition != position )
            {
                worldTransform.setPosition( position );
            }

            worldTransform.setOrientation( orientation );

            if( currentScale != scale )
            {
                worldTransform.setScale( scale );
            }

            if( getTransformReferences() == 0 )
            {
                TransformSystem::instance().setTransforms( m_handle, localTransform, worldTransform );
            }

            auto updateActorTransform = isLocalDirty() || isDirty();
            if( updateActorTransform )
            {
                auto actor = getActor();
                if( actor )
                {
                    actor->updateTransform();
                }
            }

            if( isLocalDirty() )
            {
                updateLocalFromWorld();
            }

            if( isDirty() )
            {
                updateWorldFromLocal();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    time_interval Transform::getFrameTime() const
    {
        return TransformSystem::instance().getFrameTime( m_handle );
    }

    void Transform::setFrameTime( time_interval frameTime )
    {
        TransformSystem::instance().setFrameTime( m_handle, frameTime );
    }

    time_interval Transform::getFrameDeltaTime() const
    {
        return TransformSystem::instance().getFrameDeltaTime( m_handle );
    }

    void Transform::setFrameDeltaTime( time_interval frameDeltaTime )
    {
        TransformSystem::instance().setFrameDeltaTime( m_handle, frameDeltaTime );
    }

    void Transform::updateFrameTime()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager->getTimerPtr();
        TransformSystem::instance().setFrameTimes( m_handle, timer->getTime(), timer->getDeltaTime() );
    }

    void Transform::setSmoothMotion( bool smoothMotion )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto sceneManager = (GameManager *)applicationManager->getGameManagerPtr();

        auto flags = getFlags();
        if( BitUtil::getFlagValue( flags, smoothMotionFlag ) )
        {
            sceneManager->removeSmoothTransform( this );
        }

        TransformSystem::instance().setFlag( m_handle, smoothMotionFlag, smoothMotion );

        if( BitUtil::getFlagValue( getFlags(), smoothMotionFlag ) )
        {
            sceneManager->addSmoothTransform( this );
        }
    }

    bool Transform::getSmoothMotion() const
    {
        auto flags = getFlags();
        return BitUtil::getFlagValue( flags, smoothMotionFlag );
    }

    void Transform::setTask( TaskId task )
    {
        TransformSystem::instance().setTask( m_handle, task );
    }

    TaskId Transform::getTask() const
    {
        return TransformSystem::instance().getTask( m_handle );
    }

    void Transform::lookAt( const Vector3<real_Num> &position )
    {
        auto vec = position - getPosition();
        auto rot = MathUtil<real_Num>::getRotationTo( -Vector3<real_Num>::unitZ(), vec );
        setOrientation( rot );
        setLocalDirty( true );
    }

    void Transform::lookAt( const Vector3<real_Num> &position, const Vector3<real_Num> &yawAxis )
    {
        auto vec = position - getPosition();
        auto rot = MathUtil<real_Num>::getOrientationFromDirection( vec, -Vector3<real_Num>::unitZ(),
                                                                    true, yawAxis );
        setOrientation( rot );
        setLocalDirty( true );
    }

    Vector3<real_Num> Transform::getForward() const
    {
        return getOrientation() * Vector3<real_Num>::forward();
    }

    Vector3<real_Num> Transform::getUp() const
    {
        return getOrientation() * Vector3<real_Num>::unitY();
    }

    Vector3<real_Num> Transform::getRight() const
    {
        return getOrientation() * Vector3<real_Num>::unitX();
    }

    void Transform::setFlags( u8 flags )
    {
        TransformSystem::instance().setFlags( m_handle, flags );
    }

    u8 Transform::getFlags() const
    {
        return TransformSystem::instance().getFlags( m_handle );
    }

    void Transform::setTransformReferences( s32 transformReferences )
    {
        TransformSystem::instance().setReferenceCount( m_handle, transformReferences );
    }

    s32 Transform::getTransformReferences() const
    {
        return TransformSystem::instance().getReferenceCount( m_handle );
    }

    void Transform::removeTransformReference()
    {
        TransformSystem::instance().removeReference( m_handle );
    }

    void Transform::addTransformReference()
    {
        TransformSystem::instance().addReference( m_handle );
    }

}  // namespace workphone::scene
