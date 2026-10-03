// ---------------------------------------------------------------------------
//  AssetDatabaseEditorSettings.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorSettings.hpp>
#include <Workphone/Workphone.hpp>

#include <fstream>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, AssetDatabaseEditorSettings, ISharedObject );

    namespace
    {
        String DefaultSettingsPath()
        {
            // Settings live next to the project assets so the engine's file
            // system can load them with the rest of the data.
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( applicationManager )
            {
                auto mediaPath = applicationManager->getMediaPath();
                if( !StringUtil::isNullOrEmpty( mediaPath ) )
                {
                    return mediaPath + "/adb_editor_settings.ini";
                }
            }
            return String( "adb_editor_settings.ini" );
        }

        void ReadLine( std::istream &stream, String &value )
        {
            std::string line;
            std::getline( stream, line );
            value = line;
        }

        void WriteLine( std::ostream &stream, const String &value )
        {
            stream << value << "\n";
        }
    }  // namespace

    AssetDatabaseEditorSettings::AssetDatabaseEditorSettings() = default;

    AssetDatabaseEditorSettings::~AssetDatabaseEditorSettings() = default;

    void AssetDatabaseEditorSettings::load()
    {
        std::ifstream stream( DefaultSettingsPath().c_str() );
        if( !stream )
        {
            return;
        }
        std::string header;
        std::getline( stream, header );
        if( header != "[AssetDatabaseEditor]" )
        {
            return;
        }
        ReadLine( stream, m_databasePath );
        ReadLine( stream, m_modelImportPath );
        ReadLine( stream, m_otherDatabasePath );
        ReadLine( stream, m_cachePath );
        ReadLine( stream, m_airfoilPath );
    }

    void AssetDatabaseEditorSettings::save()
    {
        std::ofstream stream( DefaultSettingsPath().c_str() );
        if( !stream )
        {
            return;
        }
        stream << "[AssetDatabaseEditor]\n";
        WriteLine( stream, m_databasePath );
        WriteLine( stream, m_modelImportPath );
        WriteLine( stream, m_otherDatabasePath );
        WriteLine( stream, m_cachePath );
        WriteLine( stream, m_airfoilPath );
    }
}  // namespace workphone::adbeditor
