#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Scene/Components/CharacterController.hpp>
#include <Workphone/Scene/Components/Collision.hpp>
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Util.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/ApplicationUtil.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, GameActor, Resource<IGameActor> );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, GameActor::FsmListener, FSMListener );

    u32 GameActor::m_idExt = 0;

    GameActor::GameActor()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        setEventTaskFlags( Thread::Application_Flag );
    }

    GameActor::GameActor( s32 id )
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        setEventTaskFlags( Thread::Application_Flag );
    }

    GameActor::~GameActor()
    {
        clearChildNodes();
        clearComponentNodes();
    }

    Array<SmartPtr<IComponent>> GameActor::snapshotComponents() const
    {
        Array<SmartPtr<IComponent>> components;
        ScopedLock lock( &m_componentsMutex, false );

        for( auto component = m_componentsHead; component != nullptr;
             component = component->m_next.load() )
        {
            components.push_back( component );
        }

        return components;
    }

    void GameActor::clearComponentNodes()
    {
        ScopedLock lock( &m_componentsMutex );

        auto component = m_componentsHead;
        m_componentsHead = nullptr;
        m_componentsTail = nullptr;

        while( component != nullptr )
        {
            auto next = component->m_next.load();
            component->m_next = nullptr;
            component->removeReference();
            component = next;
        }
    }

    void GameActor::reorderComponentNodes( const Array<SmartPtr<IComponent>> &components )
    {
        ScopedLock lock( &m_componentsMutex );

        m_componentsHead = nullptr;
        m_componentsTail = nullptr;

        for( auto component : components )
        {
            if( !component )
            {
                continue;
            }

            component->m_next = nullptr;
            if( m_componentsHead == nullptr )
            {
                m_componentsHead = component.get();
            }
            if( m_componentsTail != nullptr )
            {
                m_componentsTail->m_next = component.get();
            }
            m_componentsTail = component.get();
        }

        if( m_componentsTail == nullptr )
        {
            m_componentsHead = nullptr;
        }
    }

    Array<SmartPtr<IGameActor>> GameActor::snapshotChildren() const
    {
        Array<SmartPtr<IGameActor>> children;
        ScopedLock lock( &m_childrenMutex, false );
        children.reserve( m_numChildren );

        for( auto child = m_childrenHead; child != nullptr; child = child->nextChild.load() )
        {
            children.push_back( child );
        }

        return children;
    }

    void GameActor::clearChildNodes()
    {
        ScopedLock lock( &m_childrenMutex );

        auto child = m_childrenHead;
        m_childrenHead = nullptr;
        m_childrenTail = nullptr;
        m_numChildren = 0;

        while( child != nullptr )
        {
            auto next = child->nextChild.load();
            child->nextChild = nullptr;
            child->removeReference();
            child = next;
        }
    }

    void GameActor::reorderChildNodes( const Array<SmartPtr<IGameActor>> &children )
    {
        ScopedLock lock( &m_childrenMutex );

        m_childrenHead = nullptr;
        m_childrenTail = nullptr;
        m_numChildren = 0;

        for( auto child : children )
        {
            if( !child )
            {
                continue;
            }

            child->nextChild = nullptr;
            if( m_childrenHead == nullptr )
            {
                m_childrenHead = child.get();
            }
            if( m_childrenTail != nullptr )
            {
                m_childrenTail->nextChild = child.get();
            }
            m_childrenTail = child.get();
            ++m_numChildren;
        }

        if( m_childrenTail == nullptr )
        {
            m_childrenHead = nullptr;
        }
    }

    auto GameActor::getWorldTransform( time_interval t ) const -> Transform3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getWorldTransform();
        }

        return {};
    }

    auto GameActor::getWorldTransform() const -> Transform3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getWorldTransform();
        }

        return {};
    }

    auto GameActor::getLocalTransform( time_interval t ) const -> Transform3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getLocalTransform();
        }

        return {};
    }

    auto GameActor::getLocalTransform() const -> Transform3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getLocalTransform();
        }

        return {};
    }

    auto GameActor::getLocalPosition() const -> Vector3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getLocalPosition();
        }

        return Vector3<real_Num>::zero();
    }

    void GameActor::setLocalPosition( const Vector3<real_Num> &localPosition )
    {
        if( auto transform = getTransform() )
        {
            transform->setLocalPosition( localPosition );
            transform->setDirty( true );
        }

        updateTransform();
    }

    auto GameActor::getLocalScale() const -> Vector3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getLocalScale();
        }

        return Vector3<real_Num>::zero();
    }

    void GameActor::setLocalScale( const Vector3<real_Num> &localScale )
    {
        if( auto transform = getTransform() )
        {
            transform->setLocalScale( localScale );
            transform->setDirty( true );
        }

        updateTransform();
    }

    auto GameActor::getLocalOrientation() const -> Quaternion<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getLocalOrientation();
        }

        return Quaternion<real_Num>::identity();
    }

    void GameActor::setLocalOrientation( const Quaternion<real_Num> &localOrientation )
    {
        if( auto transform = getTransform() )
        {
            transform->setLocalOrientation( localOrientation );
            transform->setDirty( true );
        }

        updateTransform();
    }

    auto GameActor::getLocalRotation() const -> Vector3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getLocalRotation();
        }

        return Vector3<real_Num>::zero();
    }

    void GameActor::setLocalRotation( const Vector3<real_Num> &localRotation )
    {
        if( auto transform = getTransform() )
        {
            transform->setLocalRotation( localRotation );
            transform->setDirty( true );
        }

        updateTransform();
    }

    auto GameActor::getPosition() const -> Vector3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getPosition();
        }

        return Vector3<real_Num>::zero();
    }

    void GameActor::lookAt( SmartPtr<IGameActor> actor )
    {
        if( actor )
        {
            auto pos = actor->getPosition();
            lookAt( pos );
        }
    }

    void GameActor::lookAt( const Vector3<real_Num> &position )
    {
        auto vec = position - getPosition();
        auto rot = MathUtil<real_Num>::getRotationTo( -Vector3<real_Num>::unitZ(), vec );
        setOrientation( rot );
    }

    void GameActor::lookAt( const Vector3<real_Num> &position, const Vector3<real_Num> &yawAxis )
    {
        auto vec = position - getPosition();
        auto rot = MathUtil<real_Num>::getOrientationFromDirection( vec, -Vector3<real_Num>::unitZ(),
                                                                    true, yawAxis );
        setOrientation( rot );
    }

    void GameActor::setPosition( const Vector3<real_Num> &position )
    {
        if( auto transform = getTransform() )
        {
            transform->setPosition( position );
            transform->setLocalDirty( true );
        }

        updateTransform();
    }

    auto GameActor::getScale() const -> Vector3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getScale();
        }

        return Vector3<real_Num>::zero();
    }

    void GameActor::setScale( const Vector3<real_Num> &scale )
    {
        if( auto transform = getTransform() )
        {
            transform->setScale( scale );
            transform->setLocalDirty( true );
        }

        updateTransform();
    }

    auto GameActor::getOrientation() const -> Quaternion<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getOrientation();
        }

        return Quaternion<real_Num>::identity();
    }

    void GameActor::setOrientation( const Quaternion<real_Num> &orientation )
    {
        if( auto transform = getTransform() )
        {
            transform->setOrientation( orientation );
            transform->setLocalDirty( true );
        }

        updateTransform();
    }

    auto GameActor::getRotation() const -> Vector3<real_Num>
    {
        if( auto transform = getTransform() )
        {
            return transform->getRotation();
        }

        return Vector3<real_Num>::zero();
    }

    void GameActor::setRotation( const Vector3<real_Num> &rotation )
    {
        if( auto transform = getTransform() )
        {
            transform->setRotation( rotation );
            transform->setLocalDirty( true );
        }

        updateTransform();
    }

    void GameActor::levelWasLoaded( SmartPtr<IGameScene> scene )
    {
        try
        {
            ScopedLock lock( this );

            for( auto &component : getComponents() )
            {
                if( component )
                {
                    component->handleEvent( EventType::Actor, IComponent::sceneWasLoaded,
                                            Array<Parameter>(), this, component, nullptr );
                }
            }

            for( auto &child : getChildren() )
            {
                if( child )
                {
                    child->levelWasLoaded( scene );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameActor::hierarchyChanged()
    {
        try
        {
            ScopedLock lock( this );

            for( auto &component : getComponents() )
            {
                if( component )
                {
                    component->handleEvent( EventType::Actor, IComponent::hierarchyChanged,
                                            Array<Parameter>(), this, component, nullptr );
                }
            }

            for( auto &child : getChildren() )
            {
                if( child )
                {
                    child->hierarchyChanged();
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameActor::childAdded( SmartPtr<IGameActor> child )
    {
        try
        {
            ScopedLock lock( this );

            for( auto &component : getComponents() )
            {
                if( component )
                {
                    component->handleEvent( EventType::Actor, IComponent::childAdded, Array<Parameter>(),
                                            this, child, nullptr );
                }
            }

            auto children = getChildren();
            for( auto child : children )
            {
                child->childAdded( child );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameActor::childRemoved( SmartPtr<IGameActor> child )
    {
        try
        {
            ScopedLock lock( this );

            for( auto &component : getComponents() )
            {
                if( component )
                {
                    component->handleEvent( EventType::Actor, IComponent::childRemoved,
                                            Array<Parameter>(), this, child, nullptr );
                }
            }

            for( auto &child : getChildren() )
            {
                if( child )
                {
                    child->childRemoved( child );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameActor::preUpdate()
    {
        for( auto &component : getComponents() )
        {
            if( component )
            {
                component->preUpdate();
            }
        }

        for( auto &child : getChildren() )
        {
            if( child )
            {
                child->preUpdate();
            }
        }
    }

    void GameActor::updateDirty( u32 flags, u32 oldFlags )
    {
        auto components = getComponents();
        for( auto &component : components )
        {
            if( component && component->isLoaded() )
            {
                auto componentState = component->getState();
                switch( componentState )
                {
                case IComponent::State::Edit:
                case IComponent::State::Play:
                {
                    component->updateFlags( flags, oldFlags );
                }
                break;
                default:
                {
                }
                };
            }
        }

        auto children = getChildren();
        for( auto child : children )
        {
            if( child )
            {
                child->updateDirty( flags, oldFlags );
            }
        }
    }

    void GameActor::update()
    {
        for( auto &component : getComponents() )
        {
            if( component )
            {
                component->update();
            }
        }
    }

    void GameActor::postUpdate()
    {
        for( auto &component : getComponents() )
        {
            if( component )
            {
                component->postUpdate();
            }
        }

        if( isDebugDrawEnabled() )
        {
            if( auto applicationManager = core::IApplicationManager::instancePtr() )
            {
                if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
                {
                    if( auto debug = graphicsSystem->getDebugPtr() )
                    {
                        drawDebugBounds( *debug );
                    }
                }
            }
        }
    }

    void GameActor::addComponentInstance( SmartPtr<IComponent> component )
    {
        try
        {
            ScopedLock lock( this );
            ScopedLock componentsLock( &m_componentsMutex );

            WP_ASSERT( component );

            if( auto actor = component->getActor() )
            {
                if( actor.get() != this )
                {
                    WP_LOG_ERROR(
                        "GameActor::addComponentInstance: component belongs to another actor." );
                    return;
                }
            }

            for( auto current = m_componentsHead; current != nullptr; current = current->m_next.load() )
            {
                if( current == component.get() )
                {
                    WP_LOG_WARNING( "GameActor::addComponentInstance: component is already attached." );
                    return;
                }
            }

            auto componentHandle = component->getHandle();
            auto uuid = componentHandle->getUUID();
            if( uuid == UUID() )
            {
                auto newUUID = StringUtil::getUUID();
                componentHandle->setUUID( newUUID );
            }

            component->setActor( this );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto gameManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( gameManager );

            gameManager->addComponent( component );

            component->m_next = nullptr;
            component->addReference();
            if( m_componentsTail != nullptr )
            {
                m_componentsTail->m_next = component.get();
            }
            else
            {
                m_componentsHead = component.get();
            }
            m_componentsTail = component.get();

            applyCollisionMaskToComponent( component, false );
            updateBounds();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameActor::removeComponentInstance( SmartPtr<IComponent> component )
    {
        if( component )
        {
            ScopedLock lock( this );
            ScopedLock componentsLock( &m_componentsMutex );

            bool removed = false;
            IComponent *previous = nullptr;
            auto current = m_componentsHead;
            while( current != nullptr )
            {
                auto next = current->m_next.load();
                if( current == component.get() )
                {
                    if( previous != nullptr )
                    {
                        previous->m_next = next;
                    }
                    else
                    {
                        m_componentsHead = next;
                    }

                    if( m_componentsTail == current )
                    {
                        m_componentsTail = previous;
                    }

                    current->m_next = nullptr;
                    current->removeReference();
                    removed = true;
                    break;
                }

                previous = current;
                current = next;
            }

            if( !removed )
            {
                WP_LOG_WARNING(
                    "GameActor::removeComponentInstance: component is not attached to this actor." );
                return;
            }

            component->unload( nullptr );
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto sceneManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( sceneManager );

            sceneManager->removeComponent( component );
            updateBounds();
        }
        else
        {
            WP_LOG_ERROR( "Component is null" );
        }
    }

    auto GameActor::hasComponent( hash_type id ) -> bool
    {
        ScopedLock lock( this );

        for( auto &component : getComponents() )
        {
            auto pHandle = component->getHandle();
            auto componentId = pHandle->getId();

            if( componentId == id )
            {
                return true;
            }
        }

        return false;
    }

    auto GameActor::getComponent( hash_type id ) const -> SmartPtr<IComponent>
    {
        ScopedLock lock( this );

        for( auto &component : getComponents() )
        {
            auto pHandle = component->getHandle();
            auto componentId = pHandle->getId();

            if( componentId == id )
            {
                return component;
            }
        }

        return nullptr;
    }

    auto GameActor::isMine() const -> bool
    {
        ScopedLock lock( &m_flags, false );
        return BitUtil::getFlagValue( m_flags.load(), ActorFlagMine );
    }

    void GameActor::setMine( bool mine )
    {
        ScopedLock lock( &m_flags, true );
        auto newFlags = BitUtil::setFlagValue( m_flags.load(), ActorFlagMine, mine );
        setFlags( newFlags );
    }

    auto GameActor::isStatic() const -> bool
    {
        ScopedLock lock( &m_flags, false );
        return BitUtil::getFlagValue( m_flags.load(), ActorFlagStatic );
    }

    void GameActor::setStatic( bool isstatic, bool cacade )
    {
        ScopedLock lock( &m_flags, true );
        auto newFlags = BitUtil::setFlagValue( m_flags.load(), ActorFlagStatic, isstatic );
        setFlags( newFlags );

        if( cacade )
        {
            for( auto &child : getChildren() )
            {
                child->setStatic( isstatic );
            }
        }
    }

    auto GameActor::isEnabledInScene() const -> bool
    {
        auto enabledInScene = isEnabled();
        if( enabledInScene )
        {
            auto count = 0;
            auto parent = getParent();
            while( parent && count++ < 1000 )
            {
                if( !parent->isEnabled() )
                {
                    return false;
                }

                parent = parent->getParent();
            }
        }

        return enabledInScene;
    }

    auto GameActor::isEnabled() const -> bool
    {
        ScopedLock lock( &m_flags, false );
        return BitUtil::getFlagValue( m_flags.load(), ActorFlagEnabled );
    }

    void GameActor::setEnabled( bool enabled, bool cacade )
    {
        ScopedLock lock( &m_flags, true );
        auto newFlags = BitUtil::setFlagValue( m_flags.load(), ActorFlagEnabled, enabled );
        setFlags( newFlags );

        updateVisibility();

        if( cacade )
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    child->setEnabled( enabled, cacade );
                }
            }
        }
    }

    auto GameActor::isVisible() const -> bool
    {
        ScopedLock lock( &m_flags, false );
        return BitUtil::getFlagValue( m_flags.load(), ActorFlagVisible );
    }

    void GameActor::setVisible( bool visible, bool cacade )
    {
        ScopedLock lock( &m_flags, true );
        auto newFlags = BitUtil::setFlagValue( m_flags.load(), ActorFlagVisible, visible );
        setFlags( newFlags );

        if( cacade )
        {
            for( auto &child : getChildren() )
            {
                child->setVisible( visible, cacade );
            }
        }
    }

    auto GameActor::isDirty() const -> bool
    {
        ScopedLock lock( &m_flags, false );
        return BitUtil::getFlagValue( m_flags.load(), ActorFlagDirty );
    }

    void GameActor::setDirty( bool dirty, bool cacade )
    {
        ScopedLock lock( &m_flags, true );
        auto newFlags = BitUtil::setFlagValue( m_flags.load(), ActorFlagDirty, dirty );
        setFlags( newFlags );

        if( cacade )
        {
            for( auto &child : getChildren() )
            {
                child->setDirty( dirty, cacade );
            }
        }
    }

    bool GameActor::isSmoothMotion() const
    {
        ScopedLock lock( &m_flags, false );
        return BitUtil::getFlagValue( m_flags.load(), ActorFlagSmoothMotion );
    }

    void GameActor::setSmoothMotion( bool smoothMotion, bool cascade )
    {
        ScopedLock lock( &m_flags, true );
        auto newFlags = BitUtil::setFlagValue( m_flags.load(), ActorFlagSmoothMotion, smoothMotion );
        setFlags( newFlags );

        if( auto transform = getTransform() )
        {
            transform->setSmoothMotion( smoothMotion );
        }

        if( cascade )
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    child->setSmoothMotion( smoothMotion, cascade );
                }
            }
        }
    }

    u32 GameActor::getCollisionMask() const
    {
        return m_collisionMask;
    }

    void GameActor::setCollisionMask( u32 collisionMask, bool cascade )
    {
        m_collisionMask = collisionMask;
        applyCollisionMaskToPhysicsComponents( true );

        if( cascade )
        {
            for( auto &child : getChildren() )
            {
                if( child )
                {
                    child->setCollisionMask( collisionMask, cascade );
                }
            }
        }
    }

    auto GameActor::isValid() const -> bool
    {
        return true;
    }

    void GameActor::updateTransform()
    {
        if( auto transform = getTransformPtr() )
        {
            transform->update();
        }

        auto children = getChildren();
        for( auto &child : children )
        {
            if( child )
            {
                child->updateTransform();
            }
        }

        auto state = getState();
        switch( state )
        {
        case State::Edit:
        case State::Play:
        {
            auto components = getComponents();
            for( auto &component : components )
            {
                if( component )
                {
                    component->updateTransform();
                }
            }
        }
        break;
        default:
        {
        }
        }

        updateBounds();
    }

    Array<SmartPtr<IComponent>> GameActor::getComponents() const
    {
        return snapshotComponents();
    }

    void GameActor::updateComponentsState()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto sceneManager = applicationManager->getGameManagerPtr();
        auto scene = sceneManager->getCurrentScenePtr();

        auto eState = getState();
        switch( eState )
        {
        case State::Create:
        case State::Destroyed:
        case State::Edit:
        {
            auto cascade = getParent() == nullptr;

            if( auto transform = getTransform() )
            {
                transform->setDirty( true, cascade );
            }

            for( auto &component : getComponents() )
            {
                component->next = nullptr;
            }

            auto components = getComponents();
            std::sort( components.begin(), components.end(),
                       []( const SmartPtr<IComponent> &a, const SmartPtr<IComponent> &b ) {
                           return ApplicationUtil::getCreationOrder( a ) >
                                  ApplicationUtil::getCreationOrder( b );
                       } );
            reorderComponentNodes( components );

            for( auto &component : getComponents() )
            {
                if( component )
                {
                    component->setState( IComponent::State::Edit );
                }
            }

            auto children = getChildren();
            for( auto &child : children )
            {
                auto actorChild = workphone::static_pointer_cast<GameActor>( child );
                actorChild->updateComponentsState();
            }

            sceneManager->addDirtyActor( this );
        }
        break;
        case State::Play:
        {
            auto cascade = getParent() == nullptr;

            if( auto transform = getTransform() )
            {
                transform->setDirty( true, cascade );
            }

            for( auto &component : getComponents() )
            {
                if( component )
                {
                    component->setState( IComponent::State::Play );
                }
            }

            auto children = getChildren();
            for( auto &child : children )
            {
                auto actorChild = workphone::static_pointer_cast<GameActor>( child );
                actorChild->updateComponentsState();
            }

            sceneManager->addDirtyActor( this );
        }
        break;
        default:
        {
        }
        break;
        }
    }

    auto GameActor::handleActorEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        switch( eventType )
        {
        case FSMEvent::Change:
        {
        }
        break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Create:
            case State::Destroyed:
            case State::Edit:
            {
                ScopedLock lock( this, true );

                setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
                setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
                setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );

                updateComponentsState();

                updateTransform();
            }
            break;
            case State::Play:
            {
                ScopedLock lock( this, true );

                setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
                setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
                setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );

                updateComponentsState();

                updateTransform();
            }
            break;
            default:
            {
            }
            break;
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Create:
            case State::Destroyed:
            case State::Edit:
            case State::Play:
            {
            }
            break;
            default:
            {
            }
            break;
            }
        }
        break;
        case FSMEvent::Pending:
        {
        }
        break;
        case FSMEvent::Complete:
        {
        }
        break;
        case FSMEvent::NewState:
        {
        }
        break;
        case FSMEvent::WaitForChange:
        {
        }
        break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    auto GameActor::compareTag( const String &tag ) const -> bool
    {
        auto it = std::find( m_tags.begin(), m_tags.end(), tag );
        if( it != m_tags.end() )
        {
            return true;
        }

        return false;
    }

    auto GameActor::getSceneRoot() const -> SmartPtr<IGameActor>
    {
        auto parent = getParent();
        while( parent )
        {
            auto root = parent->getSceneRoot();
            if( root == nullptr )
            {
                return parent;
            }

            parent = root;
        }

        return nullptr;
    }

    IGameActor *GameActor::getSceneRootPtr() const
    {
        auto parent = getParentPtr();
        while( parent )
        {
            auto root = parent->getSceneRootPtr();
            if( root == nullptr )
            {
                return parent;
            }

            parent = root;
        }

        return nullptr;
    }

    auto GameActor::getSceneLevel() const -> u32
    {
        auto sceneLevel = 0;
        auto parent = getParentPtr();
        while( parent )
        {
            auto root = parent->getSceneRootPtr();
            if( root == nullptr )
            {
                return sceneLevel;
            }

            ++sceneLevel;
            parent = root;
        }

        return sceneLevel;
    }

    auto GameActor::getTransform() const -> SmartPtr<ITransform>
    {
        return m_transform;
    }

    void GameActor::setTransform( SmartPtr<ITransform> transform )
    {
        m_transform = transform;
    }

    void GameActor::applyCollisionMaskToComponent( SmartPtr<IComponent> component, bool force )
    {
        if( !component )
        {
            return;
        }

        auto collisionMask = getCollisionMask();

        if( component->isDerived<Rigidbody>() )
        {
            auto rigidbody = workphone::static_pointer_cast<Rigidbody>( component );
            if( force || !rigidbody->hasCollisionMaskOverride() )
            {
                rigidbody->applyActorCollisionMask( collisionMask );
            }

            collisionMask = rigidbody->getCollisionMask();

            if( auto rigidDynamic = rigidbody->getRigidDynamic() )
            {
                rigidDynamic->setCollisionMask( collisionMask );
            }

            if( auto rigidStatic = rigidbody->getRigidStatic() )
            {
                rigidStatic->setCollisionMask( collisionMask );
            }
        }

        if( component->isDerived<Collision>() )
        {
            auto collision = workphone::static_pointer_cast<Collision>( component );
            if( auto shape = collision->getShape() )
            {
                shape->setCollisionMask( collisionMask );
            }
        }

        if( component->isDerived<CharacterController>() )
        {
            auto characterController = workphone::static_pointer_cast<CharacterController>( component );
            characterController->setCollisionMask( collisionMask );
        }
    }

    void GameActor::applyCollisionMaskToPhysicsComponents( bool force )
    {
        for( auto &component : getComponents() )
        {
            applyCollisionMaskToComponent( component, force );
        }
    }

    void GameActor::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            m_flags = m_previousFlags = ActorFlagEnabled | ActorFlagVisible;

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto sceneManager = (GameManager *)applicationManager->getGameManagerPtr();
            WP_ASSERT( sceneManager );

            auto handle = getHandle();
            WP_ASSERT( handle );

            auto id = handle->getInstanceId();
            WP_ASSERT( id != std::numeric_limits<u32>::max() );

            WP_ASSERT( sceneManager->isLoaded() );

            auto transform = sceneManager->createTransform();
            setTransform( transform );

            if( transform )
            {
                transform->setActor( this );
            }

            if( data )
            {
                if( data->isDerived<Properties>() )
                {
                    fromData( data );
                }
            }
            else
            {
                for( auto &component : getComponents() )
                {
                    if( component )
                    {
                        component->load( data );
                    }
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameActor::reload( SmartPtr<ISharedObject> data )
    {
        unload( data );
        load( data );
    }

    void GameActor::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLoadstateWait loadstateWait( this );

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto gameManager = (GameManager *)applicationManager->getGameManagerPtr();
            WP_ASSERT( gameManager );

            auto transform = getTransform();

            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    child->unload( data );
                }
            }

            auto components = getComponents();
            for( auto &component : components )
            {
                try
                {
                    if( component )
                    {
                        gameManager->removeComponent( component );
                        component->unload( nullptr );
                    }
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }

            if( auto scene = getScene() )
            {
                auto pThis = getSharedFromThis<GameActor>();
                scene->unregisterAll( pThis );
            }

            m_parent = nullptr;
            setScene( nullptr );

            if( gameManager )
            {
                gameManager->destroyTransform( transform );
                setTransform( nullptr );
            }

            applicationManager->triggerEvent( EventType::Actor, IComponent::actorUnload,
                                              Array<Parameter>(), this, nullptr, nullptr );

            clearChildNodes();
            clearComponentNodes();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto GameActor::getPerpetual() const -> bool
    {
        ScopedLock lock( &m_flags, false );
        return BitUtil::getFlagValue( m_flags.load(), ActorFlagPerpetual );
    }

    void GameActor::setPerpetual( bool perpetual, bool cascade )
    {
        if( getPerpetual() != perpetual )
        {
            ScopedLock lock( &m_flags, true );

            auto newFlags = BitUtil::setFlagValue( m_flags.load(), ActorFlagPerpetual, perpetual );
            setFlags( newFlags );

            if( cascade )
            {
                for( auto child : getChildren() )
                {
                    child->setPerpetual( perpetual );
                }
            }
        }
    }

    auto GameActor::getScene() const -> SmartPtr<IGameScene>
    {
        auto p = m_scene.load();
        return p.lock();
    }

    void GameActor::setScene( SmartPtr<IGameScene> scene )
    {
        m_scene = scene;
    }

    void GameActor::triggerEnter( SmartPtr<IComponent> collision )
    {
        ScopedLock lock( this );

        for( auto &component : getComponents() )
        {
            component->handleEvent( EventType::Actor, IComponent::triggerCollisionEnter,
                                    Array<Parameter>(), this, collision, nullptr );
        }
    }

    void GameActor::triggerLeave( SmartPtr<IComponent> collision )
    {
        ScopedLock lock( this );

        for( auto &component : getComponents() )
        {
            auto args = Array<Parameter>();
            component->handleEvent( EventType::Actor, IComponent::triggerCollisionLeave, args, this,
                                    collision, nullptr );
        }
    }

    void GameActor::componentLoaded( SmartPtr<IComponent> loadedComponent )
    {
        ScopedLock lock( this );

        applyCollisionMaskToComponent( loadedComponent, false );
        updateBounds();

        for( auto &component : getComponents() )
        {
            auto args = Array<Parameter>();
            component->handleEvent( EventType::Actor, IComponent::componentLoaded, args, this,
                                    loadedComponent, nullptr );
        }
    }

    SmartPtr<IGameActor> GameActor::findChildByName( const String &name, bool cascade ) const
    {
        ScopedLock lock( this, false );

        for( auto &child : getChildren() )
        {
            if( child->getName() == name )
            {
                return child;
            }

            if( cascade )
            {
                auto foundChild = child->findChildByName( name, cascade );
                if( foundChild )
                {
                    return foundChild;
                }
            }
        }

        return nullptr;
    }

    IGameActor *GameActor::findChildByNamePtr( const String &name, bool cascade ) const
    {
        ScopedLock lock( this, false );

        for( auto &child : getChildren() )
        {
            if( child->getNamePtr() == name )
            {
                return child.get();
            }

            if( cascade )
            {
                auto foundChild = child->findChildByNamePtr( name, cascade );
                if( foundChild )
                {
                    return foundChild;
                }
            }
        }

        return nullptr;
    }

    auto GameActor::getChildByIndex( u32 index ) const -> SmartPtr<IGameActor>
    {
        ScopedLock lock( &m_childrenMutex, false );

        u32 currentIndex = 0;
        for( auto child = m_childrenHead; child != nullptr; child = child->nextChild.load() )
        {
            if( currentIndex == index )
            {
                return child;
            }
            ++currentIndex;
        }

        return nullptr;
    }

    auto GameActor::getNumChildren() const -> u32
    {
        ScopedLock lock( &m_childrenMutex, false );
        return m_numChildren;
    }

    auto GameActor::getSiblingIndex() const -> s32
    {
        ScopedLock lock( this, false );

        auto parentTransform = getParent();
        if( parentTransform == nullptr )
        {
            return 0;
        }

        const auto pThis = getSharedFromThis<IGameActor>();
        for( u32 i = 0; i < parentTransform->getNumChildren(); i++ )
        {
            const auto child = parentTransform->getChildByIndex( i );
            if( child == pThis )
            {
                return i;
            }
        }

        return -1;
    }

    void GameActor::addChild( SmartPtr<IGameActor> child )
    {
        auto eState = getState();

        if( child )
        {
            if( child.get() == this )
            {
                WP_LOG_ERROR( "GameActor::addChild: an actor cannot be its own child." );
                return;
            }

            for( auto ancestor = getParent(); ancestor != nullptr; ancestor = ancestor->getParent() )
            {
                if( ancestor == child )
                {
                    WP_LOG_ERROR(
                        "GameActor::addChild: adding an ancestor would create a hierarchy cycle." );
                    return;
                }
            }

            ScopedLock lock( this );
            ScopedLock childrenLock( &m_childrenMutex );

            for( auto current = m_childrenHead; current != nullptr; current = current->nextChild.load() )
            {
                if( current == child.get() )
                {
                    return;
                }
            }

            if( auto parent = child->getParent(); parent && parent.get() != this )
            {
                parent->removeChild( child );
            }

            auto scene = getScene();
            auto childScene = child->getScene();

            if( childScene )
            {
                if( scene != childScene )
                {
                    WP_LOG_ERROR( "Child actor already belongs to a scene." );
                }
            }

            child->setScene( scene );

            auto handle = child->getHandle();
            auto id = handle->getInstanceId();

            WP_ASSERT( id != std::numeric_limits<u32>::max() );
            //WP_ASSERT( id != getHandle()->getInstanceId() );  // child id matches parent

            child->nextChild = nullptr;
            child->addReference();
            if( m_childrenTail != nullptr )
            {
                m_childrenTail->nextChild = child.get();
            }
            else
            {
                m_childrenHead = child.get();
            }
            m_childrenTail = child.get();
            ++m_numChildren;

            child->setParent( this );

            switch( eState )
            {
            case IGameActor::State::Edit:
            case IGameActor::State::Play:
            {
                childAddedInHierarchy( child );
            }
            break;
            default:
            {
            }
            break;
            };

            child->setState( eState );
            updateBounds();
        }

        switch( eState )
        {
        case IGameActor::State::Edit:
        case IGameActor::State::Play:
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( applicationManager->isEditor() )
            {
                auto args = Array<Parameter>();
                applicationManager->triggerEvent( EventType::Actor, IComponent::childAdded, args, this,
                                                  child, nullptr );
            }
        }
        break;
        default:
        {
        }
        break;
        };
    }

    void GameActor::removeChild( SmartPtr<IGameActor> child )
    {
        if( !child )
        {
            return;
        }

        ScopedLock lock( this );
        ScopedLock childrenLock( &m_childrenMutex );

        bool removed = false;
        IGameActor *previous = nullptr;
        auto current = m_childrenHead;
        while( current != nullptr )
        {
            auto next = current->nextChild.load();
            if( current == child.get() )
            {
                if( previous != nullptr )
                {
                    previous->nextChild = next;
                }
                else
                {
                    m_childrenHead = next;
                }

                if( m_childrenTail == current )
                {
                    m_childrenTail = previous;
                }

                current->nextChild = nullptr;
                current->removeReference();
                if( m_numChildren > 0 )
                {
                    --m_numChildren;
                }
                removed = true;
                break;
            }

            previous = current;
            current = next;
        }

        if( !removed )
        {
            WP_LOG_WARNING( "GameActor::removeChild: child is not attached to this actor." );
            return;
        }

        child->setParent( nullptr );
        for( auto &component : getComponents() )
        {
            component->handleEvent( EventType::Actor, IComponent::childRemoved, Array<Parameter>(), this,
                                    child, nullptr );
        }

        for( auto &c : getChildren() )
        {
            c->childRemovedInHierarchy( child );
        }

        updateBounds();
    }

    void GameActor::childAddedInHierarchy( SmartPtr<IGameActor> child )
    {
        WP_ASSERT( child != this );

        auto childComponents = child->getComponents();
        for( auto &component : childComponents )
        {
            component->handleEvent( EventType::Actor, IComponent::childAddedInHierarchy,
                                    Array<Parameter>(), this, child, nullptr );
        }

        for( auto component : getComponents() )
        {
            component->handleEvent( EventType::Actor, IComponent::childAddedInHierarchy,
                                    Array<Parameter>(), this, child, nullptr );
        }

        for( auto &c : getChildren() )
        {
            if( child != c )
            {
                c->childAddedInHierarchy( child );
            }
        }
    }

    void GameActor::childRemovedInHierarchy( SmartPtr<IGameActor> child )
    {
        for( auto &component : getComponents() )
        {
            component->handleEvent( EventType::Actor, IComponent::childRemovedInHierarchy,
                                    Array<Parameter>(), this, child, nullptr );
        }

        for( auto &c : getChildren() )
        {
            c->childRemovedInHierarchy( child );
        }
    }

    void GameActor::removeChildren()
    {
        for( auto &child : getChildren() )
        {
            child->setParent( nullptr );
        }

        clearChildNodes();
    }

    void GameActor::destroyChildren()
    {
        auto children = getChildren();
        for( auto &child : children )
        {
            child->setParent( nullptr );
        }

        auto applicationManager = core::IApplicationManager::instance();
        auto gameManager = applicationManager->getGameManager();

        for( auto &child : children )
        {
            gameManager->destroyActor( child );
        }

        clearChildNodes();
    }

    auto GameActor::findChild( const String &name ) -> SmartPtr<IGameActor>
    {
        for( auto &child : getChildren() )
        {
            if( child->getNamePtr() == name )
            {
                return child;
            }
        }

        return nullptr;
    }

    Array<SmartPtr<IGameActor>> GameActor::getChildren() const
    {
        return snapshotChildren();
    }

    auto GameActor::getAllChildren( SmartPtr<IGameActor> parent ) const -> Array<SmartPtr<IGameActor>>
    {
        Array<SmartPtr<IGameActor>> allChildren;
        allChildren.reserve( 128 );

        auto children = parent->getChildren();
        for( auto &child : children )
        {
            allChildren.push_back( child );

            auto childChildren = getAllChildren( child );
            allChildren.insert( allChildren.end(), childChildren.begin(), childChildren.end() );
        }

        return allChildren;
    }

    auto GameActor::getAllChildren() const -> Array<SmartPtr<IGameActor>>
    {
        auto thisActor = getSharedFromThis<IGameActor>();
        return getAllChildren( thisActor );
    }

    void GameActor::setSiblingIndex( s32 index )
    {
        auto parent = getParent();
        if( parent != nullptr )
        {
            parent->setChildSiblingIndex( this, index );
        }
    }

    void GameActor::setChildSiblingIndex( SmartPtr<IGameActor> child, s32 index )
    {
        auto children = getChildren();
        if( index >= 0 && static_cast<size_t>( index ) < children.size() )
        {
            auto it = std::find( children.begin(), children.end(), child );
            if( it == children.end() )
            {
                return;
            }

            children.erase( it );
            children.insert( children.begin() + index, child );
            reorderChildNodes( children );
        }
    }

    auto GameActor::getParent() const -> SmartPtr<IGameActor>
    {
        auto p = m_parent.load();
        return p.lock();
    }

    void GameActor::setParent( SmartPtr<IGameActor> parent )
    {
        auto p = getParent();
        if( p != parent )
        {
            m_parent = parent;

            auto state = getState();
            switch( state )
            {
            case IGameActor::State::Edit:
            case IGameActor::State::Play:
            {
                if( auto transform = getTransform() )
                {
                    transform->parentChanged( parent, p );
                }

                for( auto &component : getComponents() )
                {
                    if( component )
                    {
                        Array<Parameter> arguments;
                        arguments.resize( 2 );

                        arguments[0].object = parent;
                        arguments[1].object = p;

                        component->handleEvent( EventType::Actor, IComponent::parentChanged, arguments,
                                                this, component, nullptr );
                    }
                }
            }
            break;
            default:
            {
            }
            };
        }
    }

    auto GameActor::toData() const -> SmartPtr<ISharedObject>
    {
        return GameActorUtil::toData( this );
    }

    void GameActor::fromData( SmartPtr<ISharedObject> data )
    {
        GameActorUtil::loadFromData( this, data, true );
    }

    auto GameActor::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Resource<IGameActor>::getProperties();

        properties->setProperty( GameActorUtil::labelStr, getName() );
        properties->setProperty( GameActorUtil::staticStr, isStatic() );
        properties->setProperty( GameActorUtil::enabledStr, isEnabled() );
        properties->setProperty( "visible", isVisible() );
        properties->setProperty( GameActorUtil::smoothMotionStr, isSmoothMotion() );
        properties->setProperty( GameActorUtil::collisionMaskStr, getCollisionMask() );
        properties->setProperty( GameActorUtil::layerStr, getLayer() );
        properties->setProperty( GameActorUtil::tagsStr, getTags() );

        properties->setButtonPressed( GameActorUtil::updateTransformStr );

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

        setEditorMetadata( GameActorUtil::labelStr, "Name", "Actor",
                           "Actor name shown in the scene hierarchy." );
        setEditorMetadata( GameActorUtil::enabledStr, "Enabled", "Actor",
                           "Controls actor and component updates." );
        setEditorMetadata( "visible", "Visible", "Actor",
                           "Controls actor rendering without disabling updates." );
        setEditorMetadata( GameActorUtil::staticStr, "Static", "Actor",
                           "Use a static physics body and static-scene optimizations." );
        setEditorMetadata( GameActorUtil::smoothMotionStr, "Smooth Motion", "Physics",
                           "Interpolate physics-driven transforms for rendering." );
        setEditorMetadata( GameActorUtil::collisionMaskStr, "Collision Mask", "Physics",
                           "Default collision mask inherited by physics components." );
        setEditorMetadata( GameActorUtil::layerStr, "Layer", "Organization",
                           "Scene and rendering layer for this actor." );
        setEditorMetadata( GameActorUtil::tagsStr, "Tags", "Organization",
                           "Tags used by gameplay queries and editor filtering." );
        setEditorMetadata( GameActorUtil::updateTransformStr, "Update Transform", "Actions",
                           "Push the actor transform to dependent components." );

        return properties;
    }

    void GameActor::setProperties( SmartPtr<Properties> properties )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();
        auto sceneManager = applicationManager->getGameManagerPtr();

        auto name = String();
        auto bIsStatic = false;
        auto enabled = isEnabled();
        auto visible = isVisible();
        auto smoothMotion = isSmoothMotion();
        auto collisionMask = getCollisionMask();
        auto layer = getLayer();
        auto tags = getTags();

        if( !properties->getPropertyValue( GameActorUtil::labelStr, name ) )
        {
            properties->getPropertyValue( "name", name );
        }
        properties->getPropertyValue( GameActorUtil::staticStr, bIsStatic );
        properties->getPropertyValue( GameActorUtil::enabledStr, enabled );
        properties->getPropertyValue( "visible", visible );
        properties->getPropertyValue( GameActorUtil::smoothMotionStr, smoothMotion );
        properties->getPropertyValue( GameActorUtil::collisionMaskStr, collisionMask );
        properties->getPropertyValue( GameActorUtil::layerStr, layer );
        properties->getPropertyValue( GameActorUtil::tagsStr, tags );

        if( properties->isButtonPressed( GameActorUtil::updateTransformStr ) )
        {
            updateTransform();
        }

        setName( name );
        setStatic( bIsStatic );
        setEnabled( enabled );
        setVisible( visible );
        setSmoothMotion( smoothMotion );
        setCollisionMask( collisionMask );
        setLayer( layer );
        setTags( tags );

        if( auto handle = getHandle() )
        {
            auto uuid = String();
            if( properties->getPropertyValue( GameActorUtil::uuidStr, uuid ) )
            {
                if( StringUtil::isNullOrEmpty( uuid ) )
                {
                    uuid = StringUtil::getUUID();
                }

                handle->setUUID( uuid );
            }
        }

        if( auto transform = getTransform() )
        {
            if( auto localTransformChild = properties->getChild( GameActorUtil::localTransformStr ) )
            {
                auto localTransform = transform->getLocalTransform();
                localTransform.setProperties( localTransformChild );

                transform->setLocalTransform( localTransform );
                transform->setDirty( true );
            }

            if( auto worldTransformChild = properties->getChild( GameActorUtil::worldTransformStr ) )
            {
                auto worldTransform = transform->getWorldTransform();
                worldTransform.setProperties( worldTransformChild );

                //transform->setWorldTransform( worldTransform );
            }
        }

        auto componentsData = properties->getChildrenByName( GameActorUtil::componentsStr );
        auto componentsDataAlt = properties->getChildrenByName( GameActorUtil::componentStr );
        componentsData.insert( componentsData.end(), componentsDataAlt.begin(),
                               componentsDataAlt.end() );

        auto components = Array<SmartPtr<IComponent>>();
        components.reserve( componentsData.size() );

        for( auto componentData : componentsData )
        {
            auto componentType = String();
            componentData->getPropertyValue( GameActorUtil::componentTypeStr, componentType );

            auto pComponent = factoryManager->createObjectFromType<IComponent>( componentType );
            if( !pComponent )
            {
                auto nameSplit = StringUtil::split( componentType, "::" );
                std::reverse( nameSplit.begin(), nameSplit.end() );

                for( auto componentTypeName : nameSplit )
                {
                    pComponent = factoryManager->createObjectFromType<IComponent>( componentTypeName );
                    if( pComponent )
                    {
                        break;
                    }
                }
            }

            if( !pComponent )
            {
                componentType = sceneManager->getComponentFactoryType( componentType );
                pComponent = factoryManager->createObjectFromType<IComponent>( componentType );
            }

            if( pComponent )
            {
                components.emplace_back( pComponent );
            }
        }

        for( auto &pComponent : components )
        {
            try
            {
                addComponentInstance( pComponent );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        for( size_t i = 0; i < components.size(); ++i )
        {
            try
            {
                auto &pComponent = components[i];
                auto &componentData = componentsData[i];

                if( pComponent )
                {
                    pComponent->setActor( this );
                    pComponent->fromData( componentData );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        for( size_t i = 0; i < components.size(); ++i )
        {
            try
            {
                auto &pComponent = components[i];
                auto &componentData = componentsData[i];

                if( pComponent )
                {
                    pComponent->load( nullptr );
                    componentLoaded( pComponent );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        auto childrenData = properties->getChildrenByName( GameActorUtil::childrenStr );
        auto childrenDataAlt = properties->getChildrenByName( GameActorUtil::childStr );
        childrenData.insert( childrenData.end(), childrenDataAlt.begin(), childrenDataAlt.end() );

        for( auto &childData : childrenData )
        {
            auto childActor = sceneManager->createActor();
            WP_ASSERT( childActor );

            childActor->setProperties( childData );
            addChild( childActor );
        }

        updateTransform();
    }

    void GameActor::setState( State state, bool cascade )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto pSceneManager = applicationManager->getGameManagerPtr();
        auto sceneManager = (GameManager *)pSceneManager;
        WP_ASSERT( sceneManager );

        auto changeNow = Thread::getTaskFlag( Thread::Application_Flag );

        auto scene = sceneManager->getCurrentScene();
        auto sceneLoadingState = scene->getSceneLoadingState();
        if( sceneLoadingState == IGameScene::SceneLoadingState::Loaded )
        {
            auto handle = getHandle();
            auto id = handle->getInstanceId();
            auto fsm = sceneManager->getFSMPtr( id );
            if( fsm )
            {
                fsm->setState<State>( state, changeNow );
            }

            if( cascade )
            {
                for( auto &child : getChildren() )
                {
                    child->setState( state, cascade );
                }
            }
        }
    }

    auto GameActor::getState() const -> IGameActor::State
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto pSceneManager = applicationManager->getGameManagerPtr();
        auto sceneManager = (GameManager *)pSceneManager;
        WP_ASSERT( sceneManager );

        auto handle = getHandle();
        auto id = handle->getInstanceId();
        auto fsm = sceneManager->getFSMPtr( id );
        if( fsm )
        {
            return fsm->getState<State>();
        }

        return static_cast<State>( 0 );
    }

    u32 GameActor::getPreviousFlags() const
    {
        ScopedLock lock( &m_flags, false );
        return m_previousFlags;
    }

    void GameActor::setPreviousFlags( u32 flags )
    {
        ScopedLock lock( &m_flags, true );
        m_previousFlags = flags;
    }

    auto GameActor::getFlags() const -> u32
    {
        ScopedLock lock( &m_flags, false );
        return m_flags;
    }

    void GameActor::setFlags( u32 flags )
    {
        ScopedLock lock( &m_flags, true );

        if( m_flags != flags )
        {
            m_previousFlags = m_flags;
            m_flags = flags;

            updateDirty( m_flags, m_previousFlags );
        }
    }

    auto GameActor::getFlag( u32 flag ) const -> bool
    {
        ScopedLock lock( &m_flags, false );
        return ( m_flags & flag ) != 0;
    }

    void GameActor::setFlag( u32 flag, bool value, bool cascade )
    {
        bool dirty = false;

        {
            ScopedLock lock( &m_flags, true );

            auto flags = m_flags.load();
            if( value )
            {
                flags |= flag;
            }
            else
            {
                flags &= ~flag;
            }

            if( m_flags != flags )
            {
                m_previousFlags = m_flags;
                m_flags = flags;

                dirty = true;
            }
        }

        if( dirty )
        {
            updateDirty( m_flags, m_previousFlags );
        }

        if( cascade )
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    child->setFlag( flag, value, cascade );
                }
            }
        }
    }

    void GameActor::updateVisibility()
    {
        auto enabledInScene = isEnabledInScene();
        setFlag( ActorFlagEnabledInScene, enabledInScene );

        auto children = getChildren();
        for( auto &child : children )
        {
            child->updateVisibility();
        }
    }

    void GameActor::updateOrder( bool cascade )
    {
        auto components = getComponents();
        for( auto &component : components )
        {
            component->updateOrder();
        }

        if( cascade )
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                child->updateOrder();
            }
        }
    }

    auto GameActor::handleEvent( EventType eventType, hash_type eventValue,
                                 const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                 SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) -> Parameter
    {
        if( Thread::getTaskFlag( Thread::Application_Flag ) )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto isEditing = applicationManager->isEditor() && !applicationManager->isPlaying();
            auto actorState = isEditing ? IGameActor::State::Edit : IGameActor::State::Play;
            auto componentState = isEditing ? IComponent::State::Edit : IComponent::State::Play;

            if( eventValue == IComponent::hierarchyChanged )
            {
                auto components = getComponents();
                for( auto &component : components )
                {
                    component->setState( componentState );
                }
            }
            else if( eventValue == IEvent::loadScene )
            {
                setState( actorState, false );

                auto components = getComponents();
                for( auto &component : components )
                {
                    component->setState( componentState );
                }
            }

            auto components = getComponents();
            for( auto &component : components )
            {
                component->handleEvent( eventType, eventValue, arguments, sender, object, event );
            }

            auto children = getChildren();
            for( auto &child : children )
            {
                child->handleEvent( eventType, eventValue, arguments, sender, object, event );
            }
        }

        return {};
    }

    auto GameActor::sendEvent( EventType eventType, hash_type eventValue,
                               const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) -> Parameter
    {
        ScopedLock lock( this );

        for( auto component : getComponents() )
        {
            component->handleEvent( eventType, eventValue, arguments, sender, object, event );
        }

        for( auto &child : getChildren() )
        {
            child->sendEvent( eventType, eventValue, arguments, sender, object, event );
        }

        return {};
    }

    void GameActor::lock()
    {
        auto applicationManger = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManger->getGameManagerPtr();
        gameManager->lock();
    }

    void GameActor::lock_shared()
    {
        auto applicationManger = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManger->getGameManagerPtr();
        gameManager->lock_shared();
    }

    bool GameActor::try_lock()
    {
        auto applicationManger = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManger->getGameManagerPtr();
        return gameManager->try_lock();
    }

    void GameActor::unlock()
    {
        auto applicationManger = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManger->getGameManagerPtr();
        gameManager->unlock();
    }

    void GameActor::unlock_shared()
    {
        auto applicationManger = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManger->getGameManagerPtr();
        gameManager->unlock_shared();
    }

#ifdef _DEBUG
    s32 GameActor::addReference()
    {
#    if !WP_FINAL
        //if( getObjectFlag( OBJECT_FLAG_TRACK_REFERENCES ) )
        {
            //String debugStr = getDebugStr() + DebugUtil::getStackTrace() + "\n\n";
            //setDebugStr( debugStr );
        }
#    endif

        return ISharedObject::addReference();
    }

    bool GameActor::removeReference()
    {
        return ISharedObject::removeReference();
    }
#endif

    GameActor::FsmListener::FsmListener() = default;
    GameActor::FsmListener::~FsmListener() = default;

    void GameActor::FsmListener::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loaded );
    }

    void GameActor::FsmListener::unload( SmartPtr<ISharedObject> data )
    {
        WP_ASSERT( isLoadLocked() == false );
        setLoadingState( LoadingState::Unloading );
        m_owner = nullptr;
        FSMListener::unload( nullptr );
        setLoadingState( LoadingState::Unloaded );
    }

    auto GameActor::FsmListener::handleEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        if( auto owner = getOwner() )
        {
            if( owner->isLoaded() )
            {
                return owner->handleActorEvent( state, eventType );
            }
        }

        return FSMReturnType::NotHandled;
    }

    auto GameActor::FsmListener::getOwner() const -> SmartPtr<GameActor>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void GameActor::FsmListener::setOwner( SmartPtr<GameActor> owner )
    {
        m_owner = owner;
    }

    void GameActor::clearTags()
    {
        ScopedLock lock( this, false );
        m_tags.clear();
    }

    bool GameActor::hasTag( const String &tag ) const
    {
        ScopedLock lock( this );
        return std::find( m_tags.begin(), m_tags.end(), tag ) != m_tags.end();
    }

    void GameActor::removeTag( const String &tag )
    {
        ScopedLock lock( this );
        m_tags.erase( std::remove( m_tags.begin(), m_tags.end(), tag ), m_tags.end() );
    }

    void GameActor::addTag( const String &tag )
    {
        ScopedLock lock( this );

        if( std::find( m_tags.begin(), m_tags.end(), tag ) == m_tags.end() )
        {
            m_tags.push_back( tag );
        }
    }

    void GameActor::setTags( const Array<String> &tags )
    {
        ScopedLock lock( this );

        m_tags.clear();
        m_tags.reserve( tags.size() );

        for( const auto &tag : tags )
        {
            auto trimmedTag = StringUtil::trim( tag );
            if( !StringUtil::isNullOrEmpty( trimmedTag ) &&
                std::find( m_tags.begin(), m_tags.end(), trimmedTag ) == m_tags.end() )
            {
                m_tags.push_back( trimmedTag );
            }
        }
    }

    Array<String> GameActor::getTags() const
    {
        return { m_tags.begin(), m_tags.end() };
    }

    String GameActor::getLayer() const
    {
        auto s = m_layer.load();
        return s.str();
    }

    void GameActor::setLayer( const String &layer )
    {
        m_layer = StringUtil::trim( layer );
    }

    ITransform *GameActor::getTransformPtr() const
    {
        return m_transform.get();
    }

    IGameActor *GameActor::getParentPtr() const
    {
        return m_parent.get();
    }

    IGameScene *GameActor::getScenePtr() const
    {
        return m_scene.get();
    }

    void GameActor::setRadius( f32 radius )
    {
        m_radius = radius;
    }

    f32 GameActor::getRadius() const
    {
        return m_radius;
    }

    void GameActor::setLocalAABB( const AABB3<real_Num> &localAABB )
    {
        m_localAABB = localAABB;
    }

    AABB3<real_Num> GameActor::getLocalAABB() const
    {
        return m_localAABB;
    }

    AABB3<real_Num> GameActor::getWorldAABB() const
    {
        const auto localBounds = getLocalAABB();
        if( !localBounds.isFinite() || !localBounds.isValid() )
        {
            return localBounds;
        }

        return getWorldTransform().transformAABB( localBounds );
    }

    OBB3<real_Num> GameActor::getWorldOBB() const
    {
        const auto localBounds = getLocalAABB();
        if( !localBounds.isFinite() || !localBounds.isValid() )
        {
            return {};
        }

        return OBB3<real_Num>( localBounds, getWorldTransform() );
    }

    void GameActor::drawDebugBounds( render::IDebug &debug, u32 aabbColour, u32 obbColour ) const
    {
        const auto localBounds = getLocalAABB();
        if( !localBounds.isFinite() || !localBounds.isValid() )
        {
            return;
        }

        const auto id = getId();
        debug.drawAABB( id, getWorldAABB(), aabbColour );
        debug.drawOBB( id, getWorldOBB(), obbColour );
    }

    bool GameActor::isDebugDrawEnabled() const
    {
        return m_debugDrawEnabled.load();
    }

    void GameActor::setDebugDrawEnabled( bool enabled )
    {
        m_debugDrawEnabled = enabled;
    }

    void GameActor::updateBounds()
    {
        AABB3<real_Num> localAABB;
        bool hasBounds = false;

        const auto mergeBounds = [&localAABB, &hasBounds]( const AABB3<real_Num> &bounds ) {
            if( !bounds.isFinite() || !bounds.isValid() )
            {
                return;
            }

            if( hasBounds )
            {
                localAABB.merge( bounds );
            }
            else
            {
                localAABB = bounds;
                hasBounds = true;
            }
        };

        for( const auto &component : getComponents() )
        {
            if( component )
            {
                mergeBounds( component->getBoundingBox() );
            }
        }

        for( const auto &child : getChildren() )
        {
            if( child )
            {
                const auto childBounds = child->getLocalAABB();
                if( childBounds.isFinite() && childBounds.isValid() )
                {
                    mergeBounds( child->getLocalTransform().transformAABB( childBounds ) );
                }
            }
        }

        if( hasBounds )
        {
            setLocalAABB( localAABB );
            setRadius( static_cast<f32>( localAABB.getRadius() ) );
        }
        else
        {
            localAABB.setNull();
            setLocalAABB( localAABB );
            setRadius( 0.0f );
        }
    }

}  // namespace workphone::scene
