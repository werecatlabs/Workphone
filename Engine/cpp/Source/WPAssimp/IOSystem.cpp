#include <WPAssimp/WPAssimpPCH.hpp>
#include <WPAssimp/IOSystem.hpp>
#include <WPAssimp/IOStream.hpp>
#include <Workphone/Workphone.hpp>
#include <utility>

namespace workphone
{

    IOSystem::IOSystem( const SmartPtr<IStream> &src, String grp ) :
        source( src ),
        group( std::move( grp ) )
    {
    }

    IOSystem::IOSystem() = default;

    auto IOSystem::Exists( const char *pFile ) const -> bool
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );
        WP_ASSERT( fileSystem->isValid() );

        return fileSystem->isExistingFile( pFile );
    }

    auto IOSystem::getOsSeparator() const -> char
    {
        return '/';
    }

    auto IOSystem::Open( const char *pFile, const char *pMode ) -> Assimp::IOStream *
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );
        WP_ASSERT( fileSystem->isValid() );

        auto stream = fileSystem->open( pFile );
        if( stream )
        {
            auto pStream = new IOStream( stream );
            streams.emplace_back( pStream );
            return pStream;
        }

        return nullptr;
    }

    void IOSystem::Close( Assimp::IOStream *pFile )
    {
        auto it = std::find( streams.begin(), streams.end(), pFile );
        if( it != streams.end() )
        {
            delete pFile;
            streams.erase( it );
        }
    }

}  // namespace workphone
