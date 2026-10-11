#ifndef AssetImportJob_h__
#define AssetImportJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Database/AssetCatalogBrowser.hpp>

namespace workphone
{
    namespace editor
    {
        class AssetImportJob : public Job
        {
        public:
            explicit AssetImportJob( bool reimport = false );
            ~AssetImportJob() override;

            void execute() override;

        private:
            bool m_reimport = false;
            bool m_bound = false;
            SmartPtr<IResourceDatabase> m_resources;
            SmartPtr<AssetDatabaseManager> m_catalog;
            AssetCatalogSession m_session;
            String m_projectPath, m_cachePath;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // AssetImportJob_h__
