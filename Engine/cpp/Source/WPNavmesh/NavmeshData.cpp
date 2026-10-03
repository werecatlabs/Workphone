#include "WPNavmesh/WPNavmeshPCH.hpp"
#include "WPNavmesh/NavmeshData.hpp"
#include <Workphone/IO/FileStream.hpp>
#include <Workphone/IO/BinaryStream.hpp>

namespace workphone
{

    NavmeshData::NavmeshData()
    {
    }

    NavmeshData::~NavmeshData()
    {
        m_listeners.Clear();
    }

    bool NavmeshData::LoadFromFile( const NavPath &filePath )
    {
        auto fileStream = WPCore::FileStream::OpenRead( filePath );
        if( !fileStream )
        {
            return false;
        }

        WPCore::BinaryInputStream archive( fileStream );

        // Read header to validate
        uint32_t magicNumber;
        archive >> magicNumber;

        if( magicNumber != 0x4E415643 )  // "NAVC"
        {
            return false;
        }

        // Read version
        uint32_t version;
        archive >> version;

        if( version > 1 )
        {
            return false;  // Unsupported version
        }

        // Read graph image size
        size_t imageSize;
        archive >> imageSize;

        // Read graph image data
        if( imageSize > 0 )
        {
            m_graphImage.Resize( imageSize );
            archive.Read( m_graphImage.GetData(), imageSize );
        }

        // Read bounds
        archive >> m_bounds.m_center;
        archive >> m_bounds.m_halfExtents;

        // Read layer index
        archive >> m_layerIndex;

        return true;
    }

    bool NavmeshData::SaveToFile( const WPCore::FileSystem &fileSystem ) const
    {
        auto fileStream = fileSystem.OpenWrite( "navmesh.nav" );
        if( !fileStream )
        {
            return false;
        }

        WPCore::BinaryOutputStream archive( fileStream );

        // Write header
        uint32_t magicNumber = 0x4E415643;  // "NAVC"
        archive << magicNumber;

        // Write version
        uint32_t version = 1;
        archive << version;

        // Write graph image size
        size_t imageSize = m_graphImage.GetSize();
        archive << imageSize;

        // Write graph image data
        if( imageSize > 0 )
        {
            archive.Write( m_graphImage.GetData(), imageSize );
        }

        // Write bounds
        archive << m_bounds.m_center;
        archive << m_bounds.m_halfExtents;

        // Write layer index
        archive << m_layerIndex;

        return true;
    }

    void NavmeshData::AddListener( INavmeshDataListener *listener )
    {
        if( listener && !m_listeners.Contains( listener ) )
        {
            m_listeners.Add( listener );
        }
    }

    void NavmeshData::RemoveListener( INavmeshDataListener *listener )
    {
        m_listeners.Remove( listener );
    }

}  // namespace workphone
