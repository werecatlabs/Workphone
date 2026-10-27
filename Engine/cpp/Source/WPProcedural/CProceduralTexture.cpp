#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CProceduralTexture.hpp>
#include <limits>
#include <stdexcept>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            size_t pixelCount( int width, int height )
            {
                if( width == 0 && height == 0 ) return 0;
                if( width <= 0 || height <= 0 )
                    throw std::invalid_argument( "Procedural texture dimensions must both be positive or zero" );
                const auto w = static_cast<size_t>( width ), h = static_cast<size_t>( height );
                if( w > std::numeric_limits<size_t>::max() / h ||
                    w * h > std::numeric_limits<size_t>::max() / sizeof( ColourF ) )
                    throw std::length_error( "Procedural texture pixel capacity overflow" );
                return w * h;
            }
        }

        CProceduralTexture::CProceduralTexture() = default;
        CProceduralTexture::~CProceduralTexture() = default;

        void CProceduralTexture::generate()
        {
            throw std::logic_error( "CProceduralTexture has no configured generator or native upload; use TextureBaker for CPU baking" );
        }

        void CProceduralTexture::unload( SmartPtr<ISharedObject> data )
        {
            auto pixels = std::move( m_data );
            m_width = m_height = 0;
            ISharedObject::unload( data );
        }

        void CProceduralTexture::setSize( int width, int height )
        {
            const auto count = pixelCount( width, height );
            if( count == 0 )
            {
                auto pixels = std::move( m_data );
                m_width = m_height = 0;
                return;
            }
            if( width == m_width && height == m_height ) return;
            auto candidate = makeSourceBudgetSnapshot<ColourF>();
            candidate.resize( count );
            m_data.swap( candidate );
            m_width = width;
            m_height = height;
        }

        int CProceduralTexture::getWidth() const { return m_width; }
        int CProceduralTexture::getHeight() const { return m_height; }

        void CProceduralTexture::setData( const Array<ColourF> &data )
        {
            if( data.size() != pixelCount( m_width, m_height ) )
                throw std::invalid_argument( "Procedural texture pixel count does not match its dimensions" );
            auto candidate = makeSourceBudgetSnapshot( data );
            m_data.swap( candidate );
        }

        Array<ColourF> CProceduralTexture::getData() const
        {
            return makeSourceBudgetSnapshot( m_data );
        }
    }
}
