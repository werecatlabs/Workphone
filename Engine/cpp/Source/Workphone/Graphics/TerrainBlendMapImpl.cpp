#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/TerrainBlendMapImpl.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, TerrainBlendMapImpl, ITerrainBlendMap );

    TerrainBlendMapImpl::TerrainBlendMapImpl( SmartPtr<IGraphicsTerrain> owner, u32 size, u32 layer ) :
        m_owner( owner ),
        m_size( Math<u32>::max( size, 1 ) ),
        m_layer( layer )
    {
        try
        {
            m_data.resize( static_cast<size_t>( m_size ) * m_size, 0.0f );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    TerrainBlendMapImpl::~TerrainBlendMapImpl() = default;

    void TerrainBlendMapImpl::loadImage( const String &fileName, const String &path )
    {
        // Image loading is a renderer/resource-system concern. Renderer plugins that have
        // a native blend map implementation (e.g. CTerrainOgreBlendMap) override this.
        WP_UNUSED( fileName );
        WP_UNUSED( path );
    }

    void TerrainBlendMapImpl::saveImage( const String &fileName, const String &path )
    {
        WP_UNUSED( fileName );
        WP_UNUSED( path );
    }

    f32 TerrainBlendMapImpl::getBlendValue( u32 x, u32 y )
    {
        if( m_data.empty() || x >= m_size || y >= m_size )
        {
            return 0.0f;
        }
        return m_data[static_cast<size_t>( y ) * m_size + x];
    }

    void TerrainBlendMapImpl::setBlendValue( u32 x, u32 y, f32 blendValue )
    {
        if( m_data.empty() || x >= m_size || y >= m_size )
        {
            return;
        }
        // Clamp to the valid normalised range so callers cannot corrupt the buffer.
        m_data[static_cast<size_t>( y ) * m_size + x] = Math<f32>::clamp( blendValue, 0.0f, 1.0f );
    }

    u32 TerrainBlendMapImpl::getSize() const
    {
        return m_size;
    }

    SmartPtr<IGraphicsTerrain> TerrainBlendMapImpl::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

}  // namespace workphone::render
