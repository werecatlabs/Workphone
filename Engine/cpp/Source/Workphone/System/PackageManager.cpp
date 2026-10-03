#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/PackageManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/INativeFileDialog.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Jobs/JobCreatePackage.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, PackageManager, IPackageManager );

    PackageManager::PackageManager() = default;

    PackageManager::~PackageManager() = default;

    void PackageManager::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_buildTargetPlatform.resize( (size_t)TargetPlatform::Count );
        setLoadingState( LoadingState::Loaded );
    }

    void PackageManager::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        m_buildTargetPlatform.clear();
        setLoadingState( LoadingState::Unloaded );
    }

    void PackageManager::createPackage()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();
        auto jobQueue = applicationManager->getJobQueue();

        if( auto fileDialog = fileSystem->openFileDialog() )
        {
            fileDialog->setDialogMode( INativeFileDialog::DialogMode::Select );
            fileDialog->setFileExtension( ".*" );

            auto result = fileDialog->openDialog();
            if( result == INativeFileDialog::Result::Dialog_Okay )
            {
                auto path = fileDialog->getFilePath();

                auto job = workphone::make_ptr<JobCreatePackage>();

                auto dst = StringUtil::toUTF8to16( path );
                job->setDst( dst );

                auto packageTextures = getPackageTextures();
                job->setPackageTextures( packageTextures );
                job->setBuildTargetPlatform( getBuildTargetPlatform() );

                jobQueue->addJob( job );
            }
        }
    }

    bool PackageManager::getPackageTextures() const
    {
        return m_packageTextures;
    }

    void PackageManager::setPackageTextures( bool packageTextures )
    {
        m_packageTextures = packageTextures;
    }

    Array<bool> PackageManager::getBuildTargetPlatform() const
    {
        return m_buildTargetPlatform;
    }

    void PackageManager::setBuildTargetPlatform( const Array<bool> &buildTargetPlatform )
    {
        m_buildTargetPlatform = buildTargetPlatform;
    }

    bool PackageManager::getBuildTargetPlatformEnabled( TargetPlatform targetPlatform ) const
    {
        auto index = static_cast<size_t>( targetPlatform );
        if( index >= m_buildTargetPlatform.size() )
        {
            return false;
        }

        return m_buildTargetPlatform[index];
    }

    void PackageManager::setBuildTargetPlatformEnabled( TargetPlatform targetPlatform, bool enabled )
    {
        auto index = static_cast<size_t>( targetPlatform );
        if( index >= m_buildTargetPlatform.size() )
        {
            m_buildTargetPlatform.resize( static_cast<size_t>( TargetPlatform::Count ) );
        }

        m_buildTargetPlatform[index] = enabled;
    }

}  // namespace workphone
