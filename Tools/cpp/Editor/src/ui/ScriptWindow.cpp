#include <EditorPCH.hpp>
#include <ui/ScriptWindow.hpp>
#include <editor/Project.hpp>
#include <editor/EditorManager.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/UIManager.hpp>
#include <commands/DragDropActorCmd.hpp>
#include <commands/AddActorCmd.hpp>
#include <commands/RemoveSelectionCmd.hpp>
#include <commands/PromptCmd.hpp>
#include <jobs/SceneDropJob.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ScriptWindow, EditorWindow );

    const String ScriptWindow::classNameStr = "className";
    const String ScriptWindow::updateInEditModeStr = "updateInEditMode";
    const String ScriptWindow::getPropertiesStr = "getProperties";
    const String ScriptWindow::setPropertiesStr = "setProperties";

    ScriptWindow::ScriptWindow() = default;
    ScriptWindow::~ScriptWindow() = default;

    void ScriptWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( m_changeLoadingState )
            {
                setLoadingState( LoadingState::Loading );
            }

            EditorWindow::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );
            WP_ASSERT( factoryManager->isValid() );

            auto scriptManager = applicationManager->getScriptManager();

            auto ui = applicationManager->getUI();

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            setParentWindow( parentWindow );

            if( parent )
            {
                parent->addChild( parentWindow );
            }

            auto debugWindow = ui->addElementByType<ui::IUIWindow>();
            setDebugWindow( debugWindow );
            debugWindow->setVisible( false, false );

            if( parent )
            {
                parent->addChild( debugWindow );
            }

            auto invoker = factoryManager->make_ptr<ScriptInvoker>( this );
            setInvoker( invoker );

            if( scriptManager )
            {
                WP_ASSERT( scriptManager->isValid() );

                if( !StringUtil::isNullOrEmpty( m_className ) )
                {
                    scriptManager->createObject( m_className, this );
                }
            }

            if( invoker )
            {
                invoker->callObjectMember( loadStr );
            }

            if( m_changeLoadingState )
            {
                setLoadingState( LoadingState::Loaded );
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ScriptWindow::reload( SmartPtr<ISharedObject> data )
    {
        EditorWindow::reload( data );
    }

    void ScriptWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto hasResources =
                getParentWindow() || getDebugWindow() || getInvoker() || getReceiver() ||
                getScriptClass() || getEventListener() || getWindowDropTarget();

            if( isLoaded() || hasResources )
            {
                setLoadingState( LoadingState::Unloading );

                destroyScriptObject();

                auto applicationManager = core::IApplicationManager::instancePtr();
                auto ui = applicationManager ? applicationManager->getUIPtr() : nullptr;

                if( ui )
                {
                    if( auto debugWindow = getDebugWindow() )
                    {
                        ui->removeElement( debugWindow );
                        setDebugWindow( nullptr );
                    }

                    if( auto parentWindow = getParentWindow() )
                    {
                        ui->removeElement( parentWindow );
                        setParentWindow( nullptr );
                    }
                }
                else
                {
                    setDebugWindow( nullptr );
                    setParentWindow( nullptr );
                }

                EditorWindow::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ScriptWindow::setWindowVisible( bool visible )
    {
        if( auto invoker = getInvoker() )
        {
            invoker->callObjectMember( visible ? showStr : hideStr );
        }

        EditorWindow::setWindowVisible( visible );
    }

    void ScriptWindow::setProperties( SmartPtr<Properties> properties )
    {
        if( isWindowVisible() )
        {
            if( auto invoker = getInvoker() )
            {
                auto params = Parameters();

                Parameter param;
                param.setObject( properties );

                params.push_back( param );
                invoker->callObjectMember( setPropertiesStr, params );
            }
        }
    }

    SmartPtr<Properties> ScriptWindow::getProperties() const
    {
        if( isWindowVisible() )
        {
            auto properties = EditorWindow::getProperties();

            if( auto invoker = getInvoker() )
            {
                auto params = Parameters();

                Parameter param;
                param.setObject( properties );

                params.push_back( param );
                invoker->callObjectMember( getPropertiesStr, params );
            }

            return properties;
        }

        return nullptr;
    }

}  // namespace workphone::editor
