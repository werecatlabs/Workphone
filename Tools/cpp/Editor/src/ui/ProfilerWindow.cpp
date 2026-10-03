#include <EditorPCH.hpp>
#include "ui/ProfilerWindow.hpp"
#include "editor/EditorManager.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, ProfilerWindow, EditorWindow );

    ProfilerWindow::ProfilerWindow() = default;

    ProfilerWindow::~ProfilerWindow()
    {
        unload( nullptr );
    }

    void ProfilerWindow::load( SmartPtr<ISharedObject> data )
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
                parentWindow->setLabel( "ProfilerWindowChild" );

                if( parent )
                {
                    parent->addChild( parentWindow );
                }

                auto profilerWindow = ui->addElementByType<ui::IUIProfilerWindow>();
                parentWindow->addChild( profilerWindow );
                m_profilerWindow = profilerWindow;
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ProfilerWindow::unload( SmartPtr<ISharedObject> data )
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
                    if( m_profilerWindow )
                    {
                        ui->removeElement( m_profilerWindow );
                        m_profilerWindow = nullptr;
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

    void ProfilerWindow::update()
    {
    }

    SmartPtr<ui::IUIProfilerWindow> ProfilerWindow::getProfilerWindow() const
    {
        return m_profilerWindow;
    }

    void ProfilerWindow::setProfilerWindow( SmartPtr<ui::IUIProfilerWindow> profilerWindow )
    {
        m_profilerWindow = profilerWindow;
    }
}  // namespace workphone::editor
