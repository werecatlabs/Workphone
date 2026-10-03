#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/ImportResourceJob.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Core/Path.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ImportResourceJob, Job );

    ImportResourceJob::ImportResourceJob() = default;

    ImportResourceJob::~ImportResourceJob() = default;

    void ImportResourceJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto resourceDatabase = applicationManager->getResourceDatabase();

        auto filePath = getFilePath();
        auto reimport = getReimport();

        if( Path::isFolder( filePath ) )
        {
            resourceDatabase->importFolder( filePath, reimport );
        }
        else
        {
            resourceDatabase->importFile( filePath, reimport );
        }
    }

    String ImportResourceJob::getFilePath() const
    {
        return m_filePath;
    }

    void ImportResourceJob::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }

    bool ImportResourceJob::getReimport() const
    {
        return m_reimport;
    }

    void ImportResourceJob::setReimport( bool reimport )
    {
        m_reimport = reimport;
    }

}  // namespace workphone
