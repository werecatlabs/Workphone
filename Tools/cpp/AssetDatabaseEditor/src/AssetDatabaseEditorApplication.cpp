// ---------------------------------------------------------------------------
//  AssetDatabaseEditorApplication.cpp
//
//  Default implementation of the asset database editor application.  The
//  class delegates engine bootstrap to the base @c core::Application and
//  instantiates the editor main window on top of the engine subsystems.
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorApplication.hpp>
#include <ui/AssetDatabaseEditorMainWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, AssetDatabaseEditorApplication,
                               core::Application );

    AssetDatabaseEditorApplication::AssetDatabaseEditorApplication()
    {
        static const String applicationName = "AssetDatabaseEditor";
        setName( applicationName );
    }

    AssetDatabaseEditorApplication::~AssetDatabaseEditorApplication() = default;

    void AssetDatabaseEditorApplication::load( SmartPtr<ISharedObject> data )
    {
        // Bring up the engine subsystems (graphics, ImGui UI, file system,
        // database interface, ...).  This mirrors the bootstrap the in-progress
        // Editor uses so that every interface referenced by the ported UI
        // (UIManager, AssetDatabaseManager, ...) is available.
        core::Application::load( data );

        m_mainWindow = workphone::make_ptr<AssetDatabaseEditorMainWindow>();
        m_mainWindow->load( data );
    }

    void AssetDatabaseEditorApplication::unload( SmartPtr<ISharedObject> data )
    {
        if( m_mainWindow )
        {
            m_mainWindow->unload( data );
            m_mainWindow = nullptr;
        }

        core::Application::unload( data );
    }

    void AssetDatabaseEditorApplication::run()
    {
        core::Application::run();
    }

    void AssetDatabaseEditorApplication::iterate()
    {
        core::Application::iterate();
    }
}  // namespace workphone::adbeditor
