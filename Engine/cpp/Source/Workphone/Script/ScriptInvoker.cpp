#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Script/ScriptInvoker.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Interface/Script/IScriptEvent.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <utility>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ScriptInvoker, IScriptInvoker );

    ScriptInvoker::ScriptInvoker() = default;

    ScriptInvoker::ScriptInvoker( SmartPtr<ISharedObject> scriptObject ) : m_object( scriptObject )
    {
    }

    ScriptInvoker::~ScriptInvoker()
    {
    }

    void ScriptInvoker::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        setLoadingState( LoadingState::Loaded );
    }

    void ScriptInvoker::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        setScriptData( nullptr );

        m_events.clear();
        m_object = nullptr;

        setLoadingState( LoadingState::Unloaded );
    }

    void ScriptInvoker::callObjectMember( const String &functionName )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto scriptManager = applicationManager->getScriptManagerPtr();

        if( auto object = getOwnerPtr() )
        {
            if( auto scriptData = object->getScriptDataPtr() )
            {
                scriptManager->callObjectMember( object, functionName );
            }
        }
    }

    void ScriptInvoker::callObjectMember( const String &functionName, const Parameters &params )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto scriptManager = applicationManager->getScriptManagerPtr();

        if( auto object = getOwnerPtr() )
        {
            if( auto scriptData = object->getScriptDataPtr() )
            {
                scriptManager->callObjectMember( object, functionName, params );
            }
        }
    }

    void ScriptInvoker::callObjectMember( const String &functionName, const Parameters &params,
                                          Parameters &results )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto scriptManager = applicationManager->getScriptManagerPtr();

        if( auto object = getOwnerPtr() )
        {
            if( auto scriptData = object->getScriptDataPtr() )
            {
                scriptManager->callObjectMember( object, functionName, params, results );
            }
        }
    }

    void ScriptInvoker::event( hash_type hash )
    {
        auto it = m_events.find( hash );
        if( it != m_events.end() )
        {
            const auto &event = it->second;

            auto applicationManager = core::IApplicationManager::instance();
            auto scriptManager = applicationManager->getScriptManager();
            if( !scriptManager )
            {
                auto message = String( "No script manager found. When on event class: " ) +
                               event->getClassName() + String( " function: " ) + event->getFunction();
                WP_LOG_ERROR( message );
            }

            if( auto owner = getOwner() )
            {
                if( auto scriptData = owner->getScriptData() )
                {
                    scriptManager->callObjectMember( owner, event->getFunction() );
                }
            }
            else
            {
                scriptManager->callMember( event->getClassName(), event->getFunction() );
            }
        }
    }

    void ScriptInvoker::event( hash_type hash, const Parameters &params )
    {
        auto it = m_events.find( hash );
        if( it != m_events.end() )
        {
            const auto &event = it->second;

            auto applicationManager = core::IApplicationManager::instance();
            auto scriptManager = applicationManager->getScriptManager();
            if( !scriptManager )
            {
                auto message = String( "No script manager found. When on event class: " ) +
                               event->getClassName() + String( " function: " ) + event->getFunction();

                WP_LOG_ERROR( message );
            }

            if( auto owner = getOwner() )
            {
                if( auto scriptData = owner->getScriptData() )
                {
                    scriptManager->callObjectMember( owner, event->getFunction(), params );
                }
            }
            else
            {
                scriptManager->callMember( event->getClassName(), event->getFunction(), params );
            }
        }
    }

    void ScriptInvoker::event( hash_type hash, const Parameters &params, Parameters &results )
    {
        auto it = m_events.find( hash );
        if( it != m_events.end() )
        {
            const auto &event = it->second;

            auto applicationManager = core::IApplicationManager::instance();
            auto scriptManager = applicationManager->getScriptManager();
            if( !scriptManager )
            {
                auto message = String( "No script manager found. When on event class: " ) +
                               event->getClassName() + String( " function: " ) + event->getFunction();
                WP_LOG_ERROR( message );
            }

            if( auto object = getOwner() )
            {
                if( auto scriptData = object->getScriptData() )
                {
                    scriptManager->callObjectMember( object, event->getFunction(), params, results );
                }
            }
            else
            {
                scriptManager->callMember( event->getClassName(), event->getFunction(), params,
                                           results );
            }
        }
    }

    auto ScriptInvoker::hasEvent( hash_type hash ) const -> bool
    {
        auto it = m_events.find( hash );
        if( it != m_events.end() )
        {
            return true;
        }

        return false;
    }

    void ScriptInvoker::setEventFunction( hash_type hash, SmartPtr<IEvent> event )
    {
        m_events[hash] = event;
    }

    auto ScriptInvoker::getEventFunction( hash_type hash ) const -> SmartPtr<IEvent>
    {
        auto it = m_events.find( hash );
        if( it != m_events.end() )
        {
            return it->second;
        }

        return nullptr;
    }

    void ScriptInvoker::set( hash_type hash, const Parameter &param )
    {
        auto it = m_events.find( hash );
        if( it != m_events.end() )
        {
            const auto &event = it->second;

            auto engine = core::IApplicationManager::instance();
            auto scriptMgr = engine->getScriptManager();
            if( !scriptMgr )
            {
                WP_LOG( "No script manager found." );
            }

            auto owner = getOwner();

            Parameters params( 3 );
            params.reserve( 3 );
            params[0].setPtr( owner );
            params[1] = param;

            scriptMgr->callMember( event->getClassName(), event->getFunction(), params );
        }
        else
        {
            WP_LOG_ERROR( "Script event not found." );
        }
    }

    void ScriptInvoker::set( const String &id, const Parameter &param )
    {
        auto hash = StringUtil::getHash( id );
        set( hash, param );
    }

    auto ScriptInvoker::get( hash_type hash ) -> Parameter
    {
        auto it = m_events.find( hash );
        if( it != m_events.end() )
        {
            const auto &event = it->second;

            auto engine = core::IApplicationManager::instance();
            auto scriptMgr = engine->getScriptManager();
            if( !scriptMgr )
            {
                WP_LOG( "No script manager found." );
                return Parameter::VOID_PARAM;
            }

            auto owner = getOwner();

            Parameters params;
            params[0].setPtr( owner );

            Parameters results;
            scriptMgr->callMember( event->getClassName(), event->getFunction(), params, results );

            return !results.empty() ? results[0] : Parameter::VOID_PARAM;
        }

        WP_LOG_INFO( "Script event not found." );

        return Parameter::VOID_PARAM;
    }

    auto ScriptInvoker::get( const String &id ) -> Parameter
    {
        auto hash = StringUtil::getHash( id );
        return get( hash );
    }

    auto ScriptInvoker::getNumEvents() const -> u32
    {
        return static_cast<u32>( m_events.size() );
    }

    void ScriptInvoker::setOwner( ISharedObject *owner )
    {
        m_object = owner;
    }

    auto ScriptInvoker::getOwner() const -> ISharedObject *
    {
        return m_object.get();
    }

    ISharedObject *ScriptInvoker::getOwnerPtr() const
    {
        return m_object.get();
    }

}  // namespace workphone
