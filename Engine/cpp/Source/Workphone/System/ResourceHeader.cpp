#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ResourceHeader.hpp>

namespace workphone
{

    ResourceHeader::ResourceHeader( s32 version, UUID type, u64 hash ) :
        m_version( version ),
        m_resourceType( std::move( type ) ),
        m_sourceResourceHash( hash )
    {
    }

    ResourceHeader::ResourceHeader() = default;

    void ResourceHeader::clear()
    {
        *this = ResourceHeader();
    }

    UUID ResourceHeader::getResourceTypeID() const
    {
        return m_resourceType;
    }

    void ResourceHeader::addInstallDependency( UUID resourceID )
    {
        for( auto &dep : m_installDependencies )
        {
            if( dep == resourceID )
                return;
        }

        m_installDependencies.push_back( resourceID );
    }

}  // namespace workphone
