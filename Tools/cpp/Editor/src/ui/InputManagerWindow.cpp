#include <EditorPCH.hpp>
#include <ui/InputManagerWindow.hpp>
#include "editor/EditorManager.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, InputManagerWindow, EditorWindow );

    InputManagerWindow::InputManagerWindow() = default;

    InputManagerWindow::~InputManagerWindow()
    {
        unload( nullptr );
    }

    void InputManagerWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            if( parentWindow )
            {
                setParentWindow( parentWindow );
                parentWindow->setLabel( "TerrainWindowChild" );

                if( parent )
                {
                    parent->addChild( parentWindow );
                }

                auto inputManager = ui->addElementByType<ui::IUIInputManager>();
                parentWindow->addChild( inputManager );
                m_inputManager = inputManager;
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void InputManagerWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                if( auto ui = applicationManager->getUI() )
                {
                    if( m_inputManager )
                    {
                        ui->removeElement( m_inputManager );
                        m_inputManager = nullptr;
                    }

                    if( auto parentWindow = getParentWindow() )
                    {
                        ui->removeElement( parentWindow );
                        setParentWindow( nullptr );
                    }
                }

                EditorWindow::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<ui::IUIInputManager> InputManagerWindow::getInputManager() const
    {
        return m_inputManager;
    }

    void InputManagerWindow::setInputManager( SmartPtr<ui::IUIInputManager> inputManager )
    {
        m_inputManager = inputManager;
    }
}  // namespace workphone::editor
