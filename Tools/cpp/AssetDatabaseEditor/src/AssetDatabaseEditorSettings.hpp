// ---------------------------------------------------------------------------
//  AssetDatabaseSettings.hpp
//
//  Replacement for the @c Settings.Default usage in the C# tool.  Persists a
//  handful of paths (database, model import, cache) so the user does not
//  have to re-pick them on every launch.
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorSettings_h__
#define AssetDatabaseEditorSettings_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        class AssetDatabaseEditorSettings : public ISharedObject
        {
        public:
            AssetDatabaseEditorSettings();
            ~AssetDatabaseEditorSettings() override;

            void load();
            void save();

            String getDatabasePath() const { return m_databasePath; }
            void setDatabasePath( const String &value ) { m_databasePath = value; }

            String getModelImportPath() const { return m_modelImportPath; }
            void setModelImportPath( const String &value ) { m_modelImportPath = value; }

            String getOtherDatabasePath() const { return m_otherDatabasePath; }
            void setOtherDatabasePath( const String &value ) { m_otherDatabasePath = value; }

            String getCachePath() const { return m_cachePath; }
            void setCachePath( const String &value ) { m_cachePath = value; }

            String getAirfoilPath() const { return m_airfoilPath; }
            void setAirfoilPath( const String &value ) { m_airfoilPath = value; }

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_databasePath;
            String m_modelImportPath;
            String m_otherDatabasePath;
            String m_cachePath;
            String m_airfoilPath;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorSettings_h__
