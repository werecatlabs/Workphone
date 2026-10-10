#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Script/ScriptInvoker.hpp>
#include <Workphone/Script/ScriptEvent.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Interface/Script/IScriptClass.hpp>
#include <Workphone/Interface/Script/IScriptData.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Script, Component );

    const hash_type Script::UPDATE_HASH = StringUtil::getHash( "update" );
    const hash_type Script::GET_COMPONENT_HASH = StringUtil::getHash( "getComponent" );
    const hash_type Script::ADD_COMPONENT_HASH = StringUtil::getHash( "addComponent" );
    const hash_type Script::REMOVE_COMPONENT_HASH = StringUtil::getHash( "removeComponent" );
    const hash_type Script::POSITION_HASH = StringUtil::getHash( "position" );
    const hash_type Script::ROTATION_HASH = StringUtil::getHash( "rotation" );
    const hash_type Script::SCALE_HASH = StringUtil::getHash( "scale" );

    const String Script::classNameStr = "className";
    const String Script::updateInEditModeStr = "updateInEditMode";
    const String Script::updateInPlayModeStr = "updateInPlayMode";
    const String Script::getPropertiesStr = "getProperties";
    const String Script::setPropertiesStr = "setProperties";

    Script::Script() = default;

    Script::~Script() = default;

    void Script::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager || !applicationManager->isValid() )
            {
                WP_LOG_ERROR( "ApplicationManager is null or invalid in UserComponent::load" );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto factoryManager = applicationManager->getFactoryManager();
            if( !factoryManager || !factoryManager->isValid() )
            {
                WP_LOG_ERROR( "FactoryManager is null or invalid in UserComponent::load" );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto invoker = factoryManager->make_ptr<ScriptInvoker>( this );
            if( !invoker )
            {
                WP_LOG_ERROR( "Failed to create ScriptInvoker in UserComponent::load" );
                setLoadingState( LoadingState::Error );
                return;
            }

            setInvoker( invoker );

            auto receiver = factoryManager->make_ptr<ScriptReceiver>( this );
            if( !receiver )
            {
                WP_LOG_ERROR( "Failed to create ScriptReceiver in UserComponent::load" );
                setLoadingState( LoadingState::Error );
                return;
            }

            setReceiver( receiver );

            auto updateEvent = factoryManager->make_ptr<ScriptEvent>();
            if( updateEvent )
            {
                updateEvent->setFunction( "update" );
                if( invoker )
                {
                    invoker->setEventFunction( UPDATE_HASH, updateEvent );
                }
            }
            else
            {
                WP_LOG_ERROR( "Failed to create ScriptEvent for update in UserComponent::load" );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
    }

    void Script::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager || !applicationManager->isValid() )
            {
                WP_LOG_ERROR( "ApplicationManager is null or invalid in UserComponent::unload" );
            }
            else
            {
                if( auto scriptManager = applicationManager->getScriptManager() )
                {
                    scriptManager->destroyObject( this );
                }
            }

            setInvoker( nullptr );
            setReceiver( nullptr );

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
    }

    void Script::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( data );
            load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Script::update()
    {
        auto state = getState();
        if( state == State::Play || getUpdateInEditMode() )
        {
            if( auto invoker = getInvoker() )
            {
                invoker->event( UPDATE_HASH );
            }
        }
    }

    IScriptInvoker *Script::getInvokerPtr() const
    {
        return m_invoker.get();
    }

    auto Script::getInvoker() const -> SmartPtr<IScriptInvoker>
    {
        return m_invoker;
    }

    void Script::setInvoker( SmartPtr<IScriptInvoker> invoker )
    {
        m_invoker = invoker;
    }

    auto Script::getReceiver() const -> SmartPtr<IScriptReceiver>
    {
        return m_receiver;
    }

    void Script::setReceiver( SmartPtr<IScriptReceiver> receiver )
    {
        m_receiver = receiver;
    }

    auto Script::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();
        if( !properties )
        {
            WP_LOG_ERROR( "Properties object is null in UserComponent::getProperties" );
            return nullptr;
        }

        properties->setProperty( classNameStr, m_className );
        properties->setProperty( "scriptAssetUuid", m_scriptAssetUuid );
        properties->setProperty( updateInEditModeStr, m_updateInEditMode );
        properties->setProperty( updateInPlayModeStr, m_updateInPlayMode );

        if( auto invoker = getInvoker() )
        {
            try
            {
                auto params = Parameters();

                Parameter param;
                param.setObject( properties );

                params.push_back( param );
                invoker->callObjectMember( getPropertiesStr, params );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        return properties;
    }

    void Script::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            WP_LOG_ERROR( "Properties object is null in UserComponent::setProperties" );
            return;
        }

        Component::setProperties( properties );

        try
        {
            // Authored fields must survive deserialization before attachment to an
            // actor or initialization of runtime services.
            auto className = getClassName();
            auto scriptAssetUuid = getScriptAssetUuid();
            auto updateInEditMode = getUpdateInEditMode();
            auto updateInPlayMode = getUpdateInPlayMode();
            properties->getPropertyValue( classNameStr, className );
            properties->getPropertyValue( "scriptAssetUuid", scriptAssetUuid );
            properties->getPropertyValue( updateInEditModeStr, updateInEditMode );
            properties->getPropertyValue( updateInPlayModeStr, updateInPlayMode );
            if( className != getClassName() || scriptAssetUuid != getScriptAssetUuid() )
            {
                destroyScriptData();
                setClassName( className );
                setScriptAssetUuid( scriptAssetUuid );
            }
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto sceneManager = applicationManager ? applicationManager->getGameManager() : nullptr;
            auto actor = getActor();
            if( !applicationManager || !applicationManager->isValid() || !sceneManager || !actor )
            {
                m_updateInEditMode = updateInEditMode;
                m_updateInPlayMode = updateInPlayMode;
                return;
            }

            createScriptData();

            if( auto invoker = getInvoker() )
            {
                try
                {
                    auto params = Parameters();

                    Parameter param;
                    param.setObject( properties );

                    params.push_back( param );

                    invoker->callObjectMember( setPropertiesStr, params );
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }

            auto enabled = isEnabled() && actor->isEnabledInScene();
            if( enabled )
            {
                if( m_updateInEditMode != updateInEditMode )
                {
                    m_updateInEditMode = updateInEditMode;
                    updateEditModeState();
                }

                if( m_updateInPlayMode != updateInPlayMode )
                {
                    m_updateInPlayMode = updateInPlayMode;
                    updatePlayModeState();
                }
            }
            else
            {
                m_updateInEditMode = updateInEditMode;
                m_updateInPlayMode = updateInPlayMode;

                sceneManager->unregisterAllComponent( this );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Script::getUpdateInEditMode() const -> bool
    {
        return m_updateInEditMode;
    }

    void Script::setUpdateInEditMode( bool updateInEditMode )
    {
        m_updateInEditMode = updateInEditMode;
    }

    void Script::setUpdateInPlayMode( bool updateInPlayMode )
    {
        m_updateInPlayMode = updateInPlayMode;
    }

    bool Script::getUpdateInPlayMode() const
    {
        return m_updateInPlayMode;
    }

    auto Script::getScriptClass() const -> SmartPtr<IScriptClass>
    {
        return m_scriptClass;
    }

    void Script::setScriptClass( SmartPtr<IScriptClass> scriptClass )
    {
        m_scriptClass = scriptClass;
    }

    auto Script::getClassName() const -> String
    {
        return m_className;
    }

    void Script::setClassName( const String &className )
    {
        m_className = className;
    }

    String Script::getScriptAssetUuid() const
    {
        return m_scriptAssetUuid;
    }

    void Script::setScriptAssetUuid( const String &uuid )
    {
        m_scriptAssetUuid = uuid;
    }

    auto Script::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        Component::handleComponentEvent( state, eventType );

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
            case State::Destroyed:
            {
            }
            break;
            case State::Edit:
            {
                createScriptData();
                updateEditModeState();
            }
            break;
            case State::Play:
            {
                createScriptData();
                updatePlayModeState();
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
            case State::Edit:
            {
            }
            break;
            case State::Play:
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );
                WP_ASSERT( applicationManager->isValid() );

                auto scriptManager = applicationManager->getScriptManager();
                WP_ASSERT( scriptManager );
                WP_ASSERT( scriptManager->isValid() );

                scriptManager->destroyObject( this );
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

    void Script::createScriptData()
    {
        auto scriptData = getScriptData();
        if( !scriptData )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto scriptManager = applicationManager->getScriptManager();
            if( scriptManager )
            {
                WP_ASSERT( scriptManager->isValid() );

                auto className = getClassName();
                if( !StringUtil::isNullOrEmpty( className ) )
                {
                    if( !m_scriptAssetUuid.empty() && !scriptManager->loadScriptAsset( m_scriptAssetUuid ) )
                    {
                        WP_LOG_ERROR( "Cannot load Lua script asset: " + m_scriptAssetUuid );
                        return;
                    }
                    m_scriptClass = scriptManager->createObject( className, this );
                }
            }
        }
    }

    void Script::destroyScriptData()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto scriptManager = applicationManager ? applicationManager->getScriptManager() : nullptr;
        if( scriptManager ) scriptManager->destroyObject( this );
        setScriptData( nullptr );
        m_scriptClass = nullptr;
    }

    void Script::updateEditModeState()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();

        if( m_updateInEditMode )
        {
            sceneManager->registerComponentUpdate( TaskId::Application, Thread::UpdateState::Update,
                                                   this );
        }
        else
        {
            sceneManager->unregisterAllComponent( this );
        }
    }

    void Script::updatePlayModeState()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();

        if( m_updateInPlayMode )
        {
            sceneManager->registerComponentUpdate( TaskId::Application, Thread::UpdateState::Update,
                                                   this );
        }
        else
        {
            sceneManager->unregisterAllComponent( this );
        }
    }

    void Script::generate()
    {
        if( auto invoker = getInvoker() )
        {
            invoker->callObjectMember( "generate" );
        }
    }

    Script::ScriptReceiver::ScriptReceiver() = default;

    Script::ScriptReceiver::ScriptReceiver( Script *owner ) : m_owner( owner )
    {
    }

    Script::ScriptReceiver::~ScriptReceiver() = default;

    auto Script::ScriptReceiver::setProperty( hash_type hash, void *param ) -> s32
    {
        return 0;
    }

    auto Script::ScriptReceiver::setProperty( hash_type hash, const Parameters &params ) -> s32
    {
        //if( auto owner = getOwner() )
        //{
        //    if( auto actor = owner->getActor() )
        //    {
        //        if( hash == ROTATION_HASH )
        //        {
        //            auto rotationVal = params.getParam( 0 ).getValue<Vector3<real_Num>>();
        //            actor->getTransform()->setRotation( rotationVal );
        //        }
        //        else if( hash == POSITION_HASH )
        //        {
        //            auto rotationVal = params.getParam( 0 ).getValue<Vector3<real_Num>>();
        //            actor->getTransform()->setPosition( positionVal );
        //        }
        //        else if( hash == SCALE_HASH )
        //        {
        //            auto rotationVal = params.getParam( 0 ).getValue<Vector3<real_Num>>();
        //            actor->getTransform()->setScale( scaleVal );
        //        }
        //    }
        //}

        return 0;
    }

    auto Script::ScriptReceiver::setProperty( hash_type hash, const Parameter &param ) -> s32
    {
        //if( auto owner = getOwner() )
        //{
        //    if( auto actor = owner->getActor() )
        //    {
        //        if( hash == ROTATION_HASH )
        //        {
        //            auto rotationVal = param.getValue<Vector3<real_Num>>();
        //            actor->getTransform()->setRotation( rotationVal );
        //        }
        //        else if( hash == POSITION_HASH )
        //        {
        //            auto rotationVal = param.getValue<Vector3<real_Num>>();
        //            actor->getTransform()->setPosition( positionVal );
        //        }
        //        else if( hash == SCALE_HASH )
        //        {
        //            auto rotationVal = param.getValue<Vector3<real_Num>>();
        //            actor->getTransform()->setScale( scaleVal );
        //        }
        //    }
        //}

        return 0;
    }

    auto Script::ScriptReceiver::setProperty( hash_type hash, const String &value ) -> s32
    {
        return 0;
    }

    auto Script::ScriptReceiver::getProperty( hash_type hash, void *param ) const -> s32
    {
        return 0;
    }

    auto Script::ScriptReceiver::getProperty( hash_type hash, Parameters &params ) const -> s32
    {
        //if( auto owner = getOwner() )
        //{
        //    if( auto actor = owner->getActor() )
        //    {
        //        if( hash == ROTATION_HASH )
        //        {
        //            auto rot = actor->getTransform()->getRotation();
        //            params.add( rot );
        //        }
        //        else if( hash == POSITION_HASH )
        //        {
        //            auto pos = actor->getTransform()->getPosition();
        //            params.add( pos );
        //        }
        //        else if( hash == SCALE_HASH )
        //        {
        //            auto scale = actor->getTransform()->getScale();
        //            params.add( scale );
        //        }
        //    }
        //}

        return 0;
    }

    auto Script::ScriptReceiver::getProperty( hash_type hash, Parameter &param ) const -> s32
    {
        //if( auto owner = getOwner() )
        //{
        //    if( auto actor = owner->getActor() )
        //    {
        //        if( hash == ROTATION_HASH )
        //        {
        //            auto rot = actor->getTransform()->getRotation();
        //            param.setValue( rot );
        //        }
        //        else if( hash == POSITION_HASH )
        //        {
        //            auto pos = actor->getTransform()->getPosition();
        //            param.setValue( pos );
        //        }
        //        else if( hash == SCALE_HASH )
        //        {
        //            auto scale = actor->getTransform()->getScale();
        //            param.setValue( scale );
        //        }
        //    }
        //}

        return 0;
    }

    auto Script::ScriptReceiver::getProperty( hash_type hash, String &value ) const -> s32
    {
        return 0;
    }

    auto Script::ScriptReceiver::callFunction( hash_type hash, SmartPtr<ISharedObject> object,
                                               Parameters &results ) -> s32
    {
        if( auto owner = getOwner() )
        {
            if( auto actor = owner->getActor() )
            {
            }
        }

        return 0;
    }

    auto Script::ScriptReceiver::callFunction( hash_type hash, const Parameters &params,
                                               Parameters &results ) -> s32
    {
        //if( hash == GET_COMPONENT_HASH )
        //{
        //    if( auto owner = getOwner() )
        //    {
        //        if( auto actor = owner->getActor() )
        //        {
        //            auto componentName = params.getParam( 0 ).getValue<String>();
        //            auto component = actor->getComponent( componentName );
        //            if(component)
        //            {
        //                results.add( component );
        //            }
        //        }
        //    }
        //}
        //else if( hash == ADD_COMPONENT_HASH )
        //{
        //    if( auto owner = getOwner() )
        //    {
        //        if( auto actor = owner->getActor() )
        //        {
        //            auto componentName = params.getParam( 0 ).getValue<String>();
        //            auto component = actor->addComponent( componentName );
        //            if(component)
        //            {
        //                results.add( component );
        //            }
        //        }
        //    }
        //}
        //else if( hash == REMOVE_COMPONENT_HASH )
        //{
        //    if( auto owner = getOwner() )
        //    {
        //        if( auto actor = owner->getActor() )
        //        {
        //            auto component = params.getParam( 0 ).getValue<SmartPtr<IComponent>>();
        //            actor->removeComponent( component );
        //        }
        //    }
        //}

        return 0;
    }

    auto Script::ScriptReceiver::getOwner() const -> Script *
    {
        return m_owner;
    }

    void Script::ScriptReceiver::setOwner( Script *owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::scene
